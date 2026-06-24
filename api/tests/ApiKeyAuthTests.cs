using Easure.PrintProfiles.Api;
using Xunit;

namespace Easure.PrintProfiles.Api.Tests;

public sealed class ApiKeyAuthTests
{
    [Fact]
    public void SecureEqualsReturnsTrueForMatchingKeys()
    {
        Assert.True(ApiKeyAuth.SecureEquals("test-key", "test-key"));
    }

    [Fact]
    public void SecureEqualsReturnsFalseForDifferentKeys()
    {
        Assert.False(ApiKeyAuth.SecureEquals("test-key", "wrong-key"));
    }

    [Fact]
    public void SecureEqualsReturnsFalseForDifferentLengths()
    {
        Assert.False(ApiKeyAuth.SecureEquals("test-key", "test-key-2"));
    }
}
