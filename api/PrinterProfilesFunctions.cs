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

    [Function("getPrinterProfile")]
    public async Task<IActionResult> GetProfile(
        [HttpTrigger(AuthorizationLevel.Anonymous, "get", Route = "profiles/{manufacturer}/{model}")] HttpRequest request,
        string manufacturer,
        string model)
    {
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
        [HttpTrigger(AuthorizationLevel.Function, "put", Route = "profiles/{manufacturer}/{model}")] HttpRequest request,
        string manufacturer,
        string model)
    {
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

    [Function("votePrinterProfile")]
    public async Task<IActionResult> VoteProfile(
        [HttpTrigger(AuthorizationLevel.Function, "post", Route = "profiles/{manufacturer}/{model}/vote")] HttpRequest request,
        string manufacturer,
        string model)
    {
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
        [HttpTrigger(AuthorizationLevel.Function, "get", Route = "profiles")] HttpRequest request)
    {
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
}
