using Azure;
using Azure.Data.Tables;
using Easure.PrintProfiles.Api.Models;
using Microsoft.AspNetCore.Http;
using Microsoft.AspNetCore.Mvc;
using Microsoft.Azure.Functions.Worker;

namespace Easure.PrintProfiles.Api;

public sealed class PrinterProfilesFunctions
{
    private const string DefaultTableName = "PrinterProfiles";
    private const string DefaultSubmissionTableName = "PrinterProfileSubmissions";
    private const int DefaultApprovalVoteThreshold = 3;

    [Function("getPrinterProfile")]
    public async Task<IActionResult> GetProfile(
        [HttpTrigger(AuthorizationLevel.Anonymous, "get", Route = "profiles/{manufacturer}/{model}")] HttpRequest request,
        string manufacturer,
        string model)
    {
        if (RateLimitGuard.Validate(request, "read") is { } rateLimited)
        {
            return rateLimited;
        }

        try
        {
            var (partitionKey, rowKey) = ProfileModel.TableKeys(manufacturer, model);
            var entity = await Table().GetEntityAsync<TableEntity>(partitionKey, rowKey);
            return new OkObjectResult(ProfileModel.PublicProfile(entity.Value));
        }
        catch (RequestFailedException error) when (error.Status == StatusCodes.Status404NotFound)
        {
            return new NotFoundObjectResult(new { error = "not_found" });
        }
        catch (ArgumentException error)
        {
            return new BadRequestObjectResult(new { error = "bad_request", message = error.Message });
        }
    }

    [Function("savePrinterProfile")]
    public async Task<IActionResult> SaveProfile(
        [HttpTrigger(AuthorizationLevel.Anonymous, "put", Route = "profiles/{manufacturer}/{model}")] HttpRequest request,
        string manufacturer,
        string model)
    {
        if (RateLimitGuard.Validate(request, "write") is { } rateLimited)
        {
            return rateLimited;
        }

        if (ApiKeyAuth.Validate(request) is { } unauthorized)
        {
            return unauthorized;
        }

        try
        {
            var body = await request.ReadFromJsonAsync<ProfileRequest>() ?? throw new ArgumentException("JSON body is required");
            var entity = ProfileModel.EntityFromRequest(manufacturer, model, body);
            await Table().UpsertEntityAsync(entity, TableUpdateMode.Merge);
            return new OkObjectResult(ProfileModel.PublicProfile(entity));
        }
        catch (ArgumentException error)
        {
            return new BadRequestObjectResult(new { error = "bad_request", message = error.Message });
        }
    }

    [Function("submitPrinterProfile")]
    public async Task<IActionResult> SubmitProfile(
        [HttpTrigger(AuthorizationLevel.Anonymous, "post", Route = "profile-submissions/{manufacturer}/{model}")] HttpRequest request,
        string manufacturer,
        string model)
    {
        if (RateLimitGuard.Validate(request, "write") is { } rateLimited)
        {
            return rateLimited;
        }

        try
        {
            var body = await request.ReadFromJsonAsync<ProfileRequest>() ?? throw new ArgumentException("JSON body is required");
            var submissionId = Guid.NewGuid().ToString("N");
            var now = DateTimeOffset.UtcNow;
            var entity = ProfileModel.SubmissionEntityFromRequest(manufacturer, model, body, submissionId, now);
            var canonicalTable = Table();
            var submissionTable = SubmissionTable();
            var approvalVoteThreshold = ApprovalVoteThreshold();
            var normalizedKey = Convert.ToString(entity["normalizedKey"]) ?? "";
            var canonicalKeys = ProfileModel.TableKeys(manufacturer, model);

            try
            {
                var canonical = (await canonicalTable.GetEntityAsync<TableEntity>(canonicalKeys.PartitionKey, canonicalKeys.RowKey)).Value;
                if (ProfileModel.HasSameProfileSettings(canonical, entity))
                {
                    ProfileModel.IncrementCanonicalVotes(canonical, now);
                    await canonicalTable.UpdateEntityAsync(canonical, canonical.ETag, TableUpdateMode.Merge);

                    entity["status"] = "approved";
                    entity["approvedAt"] = now.ToString("O");
                    entity["promotedProfileKey"] = canonical.RowKey;
                    await submissionTable.AddEntityAsync(entity);

                    return new OkObjectResult(new
                    {
                        submissionId,
                        normalizedKey = canonical.RowKey,
                        status = "approved",
                        promoted = false,
                        matchedCanonical = true,
                        votes = canonical["votes"]
                    });
                }
            }
            catch (RequestFailedException error) when (error.Status == StatusCodes.Status404NotFound)
            {
            }

            var matchingPending = submissionTable
                .Query<TableEntity>(stored => stored.PartitionKey == normalizedKey)
                .Where(existing =>
                    existing.RowKey != submissionId &&
                    string.Equals(Convert.ToString(existing["status"]), "pending", StringComparison.OrdinalIgnoreCase) &&
                    ProfileModel.HasSameProfileSettings(existing, entity))
                .ToArray();

            await submissionTable.AddEntityAsync(entity);

            var identicalPendingVotes = matchingPending.Length + 1;
            if (identicalPendingVotes >= approvalVoteThreshold)
            {
                var promoted = ProfileModel.PromoteSubmissionToCanonical(entity, votes: identicalPendingVotes, now);
                await canonicalTable.UpsertEntityAsync(promoted, TableUpdateMode.Merge);

                foreach (var pending in matchingPending)
                {
                    pending["status"] = "approved";
                    pending["approvedAt"] = now.ToString("O");
                    pending["promotedProfileKey"] = promoted.RowKey;
                    pending["updatedAt"] = now.ToString("O");
                    await submissionTable.UpdateEntityAsync(pending, pending.ETag, TableUpdateMode.Merge);
                }

                entity["status"] = "approved";
                entity["approvedAt"] = now.ToString("O");
                entity["promotedProfileKey"] = promoted.RowKey;
                entity["updatedAt"] = now.ToString("O");
                await submissionTable.UpdateEntityAsync(entity, entity.ETag, TableUpdateMode.Merge);

                return new OkObjectResult(new
                {
                    submissionId,
                    normalizedKey = promoted.RowKey,
                    status = "approved",
                    promoted = true,
                    matchedCanonical = false,
                    votes = identicalPendingVotes,
                    approvalVoteThreshold
                });
            }

            var hasConflictingPending = submissionTable
                .Query<TableEntity>(stored => stored.PartitionKey == normalizedKey)
                .Any(existing =>
                    existing.RowKey != submissionId &&
                    string.Equals(Convert.ToString(existing["status"]), "pending", StringComparison.OrdinalIgnoreCase) &&
                    !ProfileModel.HasSameProfileSettings(existing, entity));

            return new AcceptedResult("/api/profile-submissions/" + manufacturer + "/" + model, new
            {
                submissionId,
                normalizedKey = entity["normalizedKey"],
                status = entity["status"],
                promoted = false,
                matchedCanonical = false,
                conflict = hasConflictingPending,
                votes = identicalPendingVotes,
                approvalVoteThreshold
            });
        }
        catch (ArgumentException error)
        {
            return new BadRequestObjectResult(new { error = "bad_request", message = error.Message });
        }
    }

    [Function("votePrinterProfile")]
    public async Task<IActionResult> VoteProfile(
        [HttpTrigger(AuthorizationLevel.Anonymous, "post", Route = "profiles/{manufacturer}/{model}/vote")] HttpRequest request,
        string manufacturer,
        string model)
    {
        if (RateLimitGuard.Validate(request, "write") is { } rateLimited)
        {
            return rateLimited;
        }

        if (ApiKeyAuth.Validate(request) is { } unauthorized)
        {
            return unauthorized;
        }

        try
        {
            var (partitionKey, rowKey) = ProfileModel.TableKeys(manufacturer, model);
            var client = Table();
            var entity = (await client.GetEntityAsync<TableEntity>(partitionKey, rowKey)).Value;
            var votes = entity.TryGetValue("votes", out var votesValue) ? Convert.ToInt32(votesValue) + 1 : 1;
            entity["votes"] = votes;
            entity["updatedAt"] = DateTimeOffset.UtcNow.ToString("O");
            await client.UpdateEntityAsync(entity, entity.ETag, TableUpdateMode.Merge);
            return new OkObjectResult(new
            {
                normalizedKey = entity.TryGetValue("normalizedKey", out var normalizedKey) ? normalizedKey : "",
                votes,
                confidence = entity.TryGetValue("confidence", out var confidence) ? confidence : 0
            });
        }
        catch (RequestFailedException error) when (error.Status == StatusCodes.Status404NotFound)
        {
            return new NotFoundObjectResult(new { error = "not_found" });
        }
        catch (ArgumentException error)
        {
            return new BadRequestObjectResult(new { error = "bad_request", message = error.Message });
        }
    }

    [Function("listPrinterProfiles")]
    public IActionResult ListProfiles(
        [HttpTrigger(AuthorizationLevel.Anonymous, "get", Route = "profiles")] HttpRequest request)
    {
        if (RateLimitGuard.Validate(request, "read") is { } rateLimited)
        {
            return rateLimited;
        }

        if (ApiKeyAuth.Validate(request) is { } unauthorized)
        {
            return unauthorized;
        }

        var profiles = Table()
            .Query<TableEntity>()
            .Take(100)
            .Select(ProfileModel.PublicProfile)
            .ToArray();

        return new OkObjectResult(new { profiles });
    }

    private static TableClient Table()
    {
        var connectionString = Environment.GetEnvironmentVariable("AZURE_TABLE_CONNECTION_STRING");
        if (string.IsNullOrWhiteSpace(connectionString))
        {
            throw new InvalidOperationException("AZURE_TABLE_CONNECTION_STRING is not configured");
        }

        var tableName = Environment.GetEnvironmentVariable("PRINTER_PROFILES_TABLE");
        return new TableClient(connectionString, string.IsNullOrWhiteSpace(tableName) ? DefaultTableName : tableName);
    }

    private static TableClient SubmissionTable()
    {
        var connectionString = Environment.GetEnvironmentVariable("AZURE_TABLE_CONNECTION_STRING");
        if (string.IsNullOrWhiteSpace(connectionString))
        {
            throw new InvalidOperationException("AZURE_TABLE_CONNECTION_STRING is not configured");
        }

        var tableName = Environment.GetEnvironmentVariable("PRINTER_PROFILE_SUBMISSIONS_TABLE");
        return new TableClient(
            connectionString,
            string.IsNullOrWhiteSpace(tableName) ? DefaultSubmissionTableName : tableName);
    }

    private static int ApprovalVoteThreshold()
    {
        var configured = Environment.GetEnvironmentVariable("PRINTER_PROFILE_APPROVAL_VOTES");
        return int.TryParse(configured, out var parsed) && parsed > 1
            ? parsed
            : DefaultApprovalVoteThreshold;
    }
}
