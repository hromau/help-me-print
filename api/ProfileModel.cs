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
