using System.Text.RegularExpressions;
using Azure.Data.Tables;
using Easure.PrintProfiles.Api.Models;

namespace Easure.PrintProfiles.Api;

public static partial class ProfileModel
{
    private static readonly HashSet<string> OutputFaces = ["up", "down"];
    private static readonly HashSet<string> PageParities = ["odd", "even"];
    private static readonly HashSet<string> PageOrders = ["normal", "reverse"];
    private static readonly HashSet<string> ReloadMethods = ["same_stack", "flip_long_edge", "flip_short_edge"];

    public static string NormalizeManufacturer(string? value) => NormalizeTokenStream(value);

    public static string NormalizeModel(string? value) => NormalizeTokenStream(value);

    public static string NormalizedKey(string? manufacturer, string? model)
    {
        var normalizedManufacturer = NormalizeManufacturer(manufacturer);
        var normalizedModel = NormalizeModel(model);

        if (normalizedManufacturer.Length == 0)
        {
            return normalizedModel;
        }

        if (normalizedModel.Length == 0)
        {
            return normalizedManufacturer;
        }

        return $"{normalizedManufacturer}:{normalizedModel}";
    }

    public static (string PartitionKey, string RowKey) TableKeys(string? manufacturer, string? model)
    {
        var partitionKey = NormalizeManufacturer(manufacturer);
        var rowKey = NormalizedKey(manufacturer, model);

        if (partitionKey.Length == 0 || rowKey.Length == 0)
        {
            throw new ArgumentException("manufacturer and model are required");
        }

        return (partitionKey, rowKey);
    }

    public static TableEntity EntityFromRequest(string manufacturer, string model, ProfileRequest request, DateTimeOffset? now = null)
    {
        var timestamp = now ?? DateTimeOffset.UtcNow;
        var (partitionKey, rowKey) = TableKeys(manufacturer, model);
        var displayName = string.IsNullOrWhiteSpace(request.DisplayName)
            ? $"{manufacturer} {model}".Trim()
            : request.DisplayName.Trim();

        return new TableEntity(partitionKey, rowKey)
        {
            ["displayName"] = displayName,
            ["manufacturer"] = string.IsNullOrWhiteSpace(request.Manufacturer) ? manufacturer.Trim() : request.Manufacturer.Trim(),
            ["model"] = string.IsNullOrWhiteSpace(request.Model) ? model.Trim() : request.Model.Trim(),
            ["normalizedKey"] = rowKey,
            ["outputFace"] = RequireValue(request.OutputFace, "outputFace", OutputFaces),
            ["firstPassParity"] = RequireValue(request.FirstPassParity, "firstPassParity", PageParities),
            ["secondPassParity"] = RequireValue(request.SecondPassParity, "secondPassParity", PageParities),
            ["firstPassOrder"] = RequireValue(request.FirstPassOrder, "firstPassOrder", PageOrders),
            ["evenPagesOrder"] = RequireValue(request.EvenPagesOrder, "evenPagesOrder", PageOrders),
            ["reloadMethod"] = RequireValue(request.ReloadMethod, "reloadMethod", ReloadMethods),
            ["confidence"] = Math.Clamp(request.Confidence ?? 80, 0, 100),
            ["votes"] = Math.Max(0, request.Votes ?? 1),
            ["createdAt"] = string.IsNullOrWhiteSpace(request.CreatedAt) ? timestamp.ToString("O") : request.CreatedAt,
            ["updatedAt"] = timestamp.ToString("O"),
            ["source"] = "cloud",
            ["schemaVersion"] = 1
        };
    }

    public static TableEntity SubmissionEntityFromRequest(
        string manufacturer,
        string model,
        ProfileRequest request,
        string submissionId,
        DateTimeOffset? now = null)
    {
        var timestamp = now ?? DateTimeOffset.UtcNow;
        var canonical = EntityFromRequest(manufacturer, model, request, timestamp);

        return new TableEntity(canonical.RowKey, submissionId)
        {
            ["displayName"] = canonical["displayName"],
            ["manufacturer"] = canonical["manufacturer"],
            ["model"] = canonical["model"],
            ["normalizedKey"] = canonical["normalizedKey"],
            ["outputFace"] = canonical["outputFace"],
            ["firstPassParity"] = canonical["firstPassParity"],
            ["secondPassParity"] = canonical["secondPassParity"],
            ["firstPassOrder"] = canonical["firstPassOrder"],
            ["evenPagesOrder"] = canonical["evenPagesOrder"],
            ["reloadMethod"] = canonical["reloadMethod"],
            ["confidence"] = canonical["confidence"],
            ["votes"] = canonical["votes"],
            ["createdAt"] = canonical["createdAt"],
            ["updatedAt"] = canonical["updatedAt"],
            ["source"] = "submission",
            ["schemaVersion"] = canonical["schemaVersion"],
            ["status"] = "pending",
            ["submissionSource"] = "desktop"
        };
    }

    public static ProfileDto PublicProfile(TableEntity entity)
    {
        return new ProfileDto(
            ReadString(entity, "displayName"),
            ReadString(entity, "manufacturer"),
            ReadString(entity, "model"),
            ReadString(entity, "normalizedKey"),
            ReadString(entity, "outputFace"),
            ReadString(entity, "firstPassParity"),
            ReadString(entity, "secondPassParity"),
            ReadString(entity, "firstPassOrder"),
            ReadString(entity, "evenPagesOrder"),
            ReadString(entity, "reloadMethod"),
            ReadInt(entity, "confidence"),
            ReadInt(entity, "votes"),
            ReadString(entity, "source"),
            ReadInt(entity, "schemaVersion", 1),
            ReadString(entity, "updatedAt"));
    }

    public static bool HasSameProfileSettings(TableEntity left, TableEntity right)
    {
        return ReadString(left, "normalizedKey") == ReadString(right, "normalizedKey") &&
            ReadString(left, "outputFace") == ReadString(right, "outputFace") &&
            ReadString(left, "firstPassParity") == ReadString(right, "firstPassParity") &&
            ReadString(left, "secondPassParity") == ReadString(right, "secondPassParity") &&
            ReadString(left, "firstPassOrder") == ReadString(right, "firstPassOrder") &&
            ReadString(left, "evenPagesOrder") == ReadString(right, "evenPagesOrder") &&
            ReadString(left, "reloadMethod") == ReadString(right, "reloadMethod");
    }

    public static TableEntity PromoteSubmissionToCanonical(TableEntity submission, int votes, DateTimeOffset? now = null)
    {
        var timestamp = now ?? DateTimeOffset.UtcNow;
        var partitionKey = NormalizeManufacturer(ReadString(submission, "manufacturer"));
        var rowKey = ReadString(submission, "normalizedKey");
        if (partitionKey.Length == 0 || rowKey.Length == 0)
        {
            throw new ArgumentException("submission must contain manufacturer and normalizedKey");
        }

        return new TableEntity(partitionKey, rowKey)
        {
            ["displayName"] = ReadString(submission, "displayName"),
            ["manufacturer"] = ReadString(submission, "manufacturer"),
            ["model"] = ReadString(submission, "model"),
            ["normalizedKey"] = rowKey,
            ["outputFace"] = ReadString(submission, "outputFace"),
            ["firstPassParity"] = ReadString(submission, "firstPassParity"),
            ["secondPassParity"] = ReadString(submission, "secondPassParity"),
            ["firstPassOrder"] = ReadString(submission, "firstPassOrder"),
            ["evenPagesOrder"] = ReadString(submission, "evenPagesOrder"),
            ["reloadMethod"] = ReadString(submission, "reloadMethod"),
            ["confidence"] = ReadInt(submission, "confidence", 80),
            ["votes"] = votes,
            ["createdAt"] = ReadString(submission, "createdAt"),
            ["updatedAt"] = timestamp.ToString("O"),
            ["source"] = "cloud",
            ["schemaVersion"] = ReadInt(submission, "schemaVersion", 1)
        };
    }

    public static TableEntity IncrementCanonicalVotes(TableEntity canonical, DateTimeOffset? now = null)
    {
        var timestamp = now ?? DateTimeOffset.UtcNow;
        canonical["votes"] = ReadInt(canonical, "votes", 0) + 1;
        canonical["updatedAt"] = timestamp.ToString("O");
        return canonical;
    }

    private static string ReadString(TableEntity entity, string name)
    {
        return entity.TryGetValue(name, out var value) ? Convert.ToString(value) ?? "" : "";
    }

    private static int ReadInt(TableEntity entity, string name, int fallback = 0)
    {
        if (!entity.TryGetValue(name, out var value))
        {
            return fallback;
        }

        return value switch
        {
            int number => number,
            long number => (int)number,
            double number => (int)number,
            string text when int.TryParse(text, out var parsed) => parsed,
            _ => fallback
        };
    }

    private static string RequireValue(string? value, string field, HashSet<string> allowed)
    {
        if (value is null || !allowed.Contains(value))
        {
            throw new ArgumentException($"{field} must be one of: {string.Join(", ", allowed)}");
        }

        return value;
    }

    private static string NormalizeTokenStream(string? value)
    {
        var normalized = NonAlphaNumeric().Replace((value ?? "").Trim().ToLowerInvariant(), " ");
        return Whitespace().Replace(normalized, " ").Trim();
    }

    [GeneratedRegex("[^a-z0-9]+")]
    private static partial Regex NonAlphaNumeric();

    [GeneratedRegex("\\s+")]
    private static partial Regex Whitespace();
}
