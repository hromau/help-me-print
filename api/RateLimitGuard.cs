using System.Collections.Concurrent;
using Microsoft.AspNetCore.Http;
using Microsoft.AspNetCore.Mvc;

namespace Easure.PrintProfiles.Api;

internal static class RateLimitGuard
{
    private const int DefaultReadLimit = 120;
    private const int DefaultWriteLimit = 30;
    private const int DefaultWindowSeconds = 60;

    private static readonly ConcurrentDictionary<string, Counter> Counters = new();

    public static IActionResult? Validate(HttpRequest request, string bucket)
    {
        var limit = bucket.Equals("write", StringComparison.OrdinalIgnoreCase)
            ? IntSetting("RATE_LIMIT_WRITE_REQUESTS", DefaultWriteLimit)
            : IntSetting("RATE_LIMIT_READ_REQUESTS", DefaultReadLimit);

        var windowSeconds = IntSetting("RATE_LIMIT_WINDOW_SECONDS", DefaultWindowSeconds);
        var window = TimeSpan.FromSeconds(windowSeconds);
        var now = DateTimeOffset.UtcNow;
        var clientId = ClientId(request);
        var key = bucket + ":" + clientId;

        var counter = Counters.AddOrUpdate(
            key,
            _ => new Counter(1, now),
            (_, current) =>
            {
                if (now - current.WindowStartedAt >= window)
                {
                    return new Counter(1, now);
                }

                return current with { Count = current.Count + 1 };
            });

        if (counter.Count <= limit)
        {
            return null;
        }

        var retryAfter = Math.Max(1, windowSeconds - (int)(now - counter.WindowStartedAt).TotalSeconds);
        request.HttpContext.Response.Headers["Retry-After"] = retryAfter.ToString();
        return new ObjectResult(new
        {
            error = "rate_limited",
            bucket,
            limit,
            windowSeconds,
            retryAfterSeconds = retryAfter
        })
        {
            StatusCode = StatusCodes.Status429TooManyRequests
        };
    }

    private static string ClientId(HttpRequest request)
    {
        if (request.Headers.TryGetValue("X-Forwarded-For", out var forwardedFor))
        {
            var raw = forwardedFor.ToString();
            var first = raw.Split(',', StringSplitOptions.TrimEntries | StringSplitOptions.RemoveEmptyEntries).FirstOrDefault();
            if (!string.IsNullOrWhiteSpace(first))
            {
                return first;
            }
        }

        return request.HttpContext.Connection.RemoteIpAddress?.ToString() ?? "unknown";
    }

    private static int IntSetting(string name, int fallback)
    {
        var raw = Environment.GetEnvironmentVariable(name);
        return int.TryParse(raw, out var parsed) && parsed > 0 ? parsed : fallback;
    }

    private sealed record Counter(int Count, DateTimeOffset WindowStartedAt);
}
