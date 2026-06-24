using System.Security.Cryptography;
using System.Text;
using Microsoft.AspNetCore.Http;
using Microsoft.AspNetCore.Mvc;

namespace Easure.PrintProfiles.Api;

internal static class ApiKeyAuth
{
    private const string ApiKeyHeaderName = "X-Api-Key";
    private const string ApiKeyEnvironmentVariable = "API_ACCESS_KEY";

    public static IActionResult? Validate(HttpRequest request)
    {
        var configuredKey = Environment.GetEnvironmentVariable(ApiKeyEnvironmentVariable);
        if (string.IsNullOrWhiteSpace(configuredKey))
        {
            return new ObjectResult(new
            {
                error = "server_misconfigured",
                message = $"{ApiKeyEnvironmentVariable} is not configured"
            })
            {
                StatusCode = StatusCodes.Status500InternalServerError
            };
        }

        if (!request.Headers.TryGetValue(ApiKeyHeaderName, out var providedValues))
        {
            return Unauthorized("missing_api_key");
        }

        var providedKey = providedValues.ToString();
        if (string.IsNullOrWhiteSpace(providedKey) || !SecureEquals(configuredKey, providedKey))
        {
            return Unauthorized("invalid_api_key");
        }

        return null;
    }

    internal static bool SecureEquals(string left, string right)
    {
        var leftBytes = Encoding.UTF8.GetBytes(left);
        var rightBytes = Encoding.UTF8.GetBytes(right);
        return CryptographicOperations.FixedTimeEquals(leftBytes, rightBytes);
    }

    private static UnauthorizedObjectResult Unauthorized(string error) => new(new { error });
}
