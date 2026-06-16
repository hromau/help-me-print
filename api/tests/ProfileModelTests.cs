using Easure.PrintProfiles.Api;
using Easure.PrintProfiles.Api.Models;
using Xunit;

namespace Easure.PrintProfiles.Api.Tests;

public sealed class ProfileModelTests
{
    [Fact]
    public void NormalizesManufacturerAndModelIntoStableTableKeys()
    {
        Assert.Equal("hp:smart tank 580", ProfileModel.NormalizedKey("HP", " Smart  Tank-580 "));
        Assert.Equal(("hp", "hp:smart tank 580"), ProfileModel.TableKeys("HP", "Smart Tank 580"));
    }

    [Fact]
    public void BuildsValidTableEntityFromProfilePayload()
    {
        var entity = ProfileModel.EntityFromRequest("HP", "Smart Tank 580", new ProfileRequest(
            DisplayName: null,
            Manufacturer: null,
            Model: null,
            OutputFace: "up",
            FirstPassParity: "even",
            SecondPassParity: "odd",
            FirstPassOrder: "normal",
            EvenPagesOrder: "reverse",
            ReloadMethod: "same_stack",
            Confidence: 98,
            Votes: null,
            CreatedAt: null), DateTimeOffset.Parse("2026-06-14T00:00:00.000Z"));

        Assert.Equal("hp", entity.PartitionKey);
        Assert.Equal("hp:smart tank 580", entity.RowKey);
        Assert.Equal("hp:smart tank 580", entity["normalizedKey"]);
        Assert.Equal("cloud", entity["source"]);
        Assert.Equal(1, entity["schemaVersion"]);
        Assert.Equal("2026-06-14T00:00:00.0000000+00:00", entity["updatedAt"]);
    }

    [Fact]
    public void RejectsInvalidEnumValues()
    {
        var error = Assert.Throws<ArgumentException>(() => ProfileModel.EntityFromRequest("HP", "Smart Tank 580", new ProfileRequest(
            DisplayName: null,
            Manufacturer: null,
            Model: null,
            OutputFace: "sideways",
            FirstPassParity: "even",
            SecondPassParity: "odd",
            FirstPassOrder: "normal",
            EvenPagesOrder: "reverse",
            ReloadMethod: "same_stack",
            Confidence: null,
            Votes: null,
            CreatedAt: null)));

        Assert.Contains("outputFace", error.Message);
    }
}
