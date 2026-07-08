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

    [Fact]
    public void BuildsPendingSubmissionEntityFromProfilePayload()
    {
        var entity = ProfileModel.SubmissionEntityFromRequest("HP", "Smart Tank 580", new ProfileRequest(
            DisplayName: "HP Smart Tank 580",
            Manufacturer: "HP",
            Model: "Smart Tank 580",
            OutputFace: "up",
            FirstPassParity: "even",
            SecondPassParity: "odd",
            FirstPassOrder: "normal",
            EvenPagesOrder: "reverse",
            ReloadMethod: "same_stack",
            Confidence: 80,
            Votes: 1,
            CreatedAt: "2026-06-14T00:00:00.000Z"), "submission-1", DateTimeOffset.Parse("2026-06-14T00:00:00.000Z"));

        Assert.Equal("hp:smart tank 580", entity.PartitionKey);
        Assert.Equal("submission-1", entity.RowKey);
        Assert.Equal("pending", entity["status"]);
        Assert.Equal("submission", entity["source"]);
        Assert.Equal("desktop", entity["submissionSource"]);
    }

    [Fact]
    public void DetectsMatchingSubmissionSettings()
    {
        var left = ProfileModel.SubmissionEntityFromRequest("HP", "Smart Tank 580", new ProfileRequest(
            DisplayName: "HP Smart Tank 580",
            Manufacturer: "HP",
            Model: "Smart Tank 580",
            OutputFace: "up",
            FirstPassParity: "even",
            SecondPassParity: "odd",
            FirstPassOrder: "normal",
            EvenPagesOrder: "reverse",
            ReloadMethod: "same_stack",
            Confidence: 80,
            Votes: 1,
            CreatedAt: "2026-06-14T00:00:00.000Z"), "submission-1", DateTimeOffset.Parse("2026-06-14T00:00:00.000Z"));
        var right = ProfileModel.SubmissionEntityFromRequest("HP", "Smart Tank 580", new ProfileRequest(
            DisplayName: "HP Smart Tank 580 via driver B",
            Manufacturer: "HP",
            Model: "Smart Tank 580",
            OutputFace: "up",
            FirstPassParity: "even",
            SecondPassParity: "odd",
            FirstPassOrder: "normal",
            EvenPagesOrder: "reverse",
            ReloadMethod: "same_stack",
            Confidence: 90,
            Votes: 1,
            CreatedAt: "2026-06-15T00:00:00.000Z"), "submission-2", DateTimeOffset.Parse("2026-06-15T00:00:00.000Z"));

        Assert.True(ProfileModel.HasSameProfileSettings(left, right));
    }

    [Fact]
    public void DetectsConflictingSubmissionSettings()
    {
        var left = ProfileModel.SubmissionEntityFromRequest("HP", "Smart Tank 580", new ProfileRequest(
            DisplayName: "HP Smart Tank 580",
            Manufacturer: "HP",
            Model: "Smart Tank 580",
            OutputFace: "up",
            FirstPassParity: "even",
            SecondPassParity: "odd",
            FirstPassOrder: "normal",
            EvenPagesOrder: "reverse",
            ReloadMethod: "same_stack",
            Confidence: 80,
            Votes: 1,
            CreatedAt: "2026-06-14T00:00:00.000Z"), "submission-1", DateTimeOffset.Parse("2026-06-14T00:00:00.000Z"));
        var right = ProfileModel.SubmissionEntityFromRequest("HP", "Smart Tank 580", new ProfileRequest(
            DisplayName: "HP Smart Tank 580 alternate",
            Manufacturer: "HP",
            Model: "Smart Tank 580",
            OutputFace: "down",
            FirstPassParity: "odd",
            SecondPassParity: "even",
            FirstPassOrder: "reverse",
            EvenPagesOrder: "normal",
            ReloadMethod: "flip_long_edge",
            Confidence: 80,
            Votes: 1,
            CreatedAt: "2026-06-15T00:00:00.000Z"), "submission-2", DateTimeOffset.Parse("2026-06-15T00:00:00.000Z"));

        Assert.False(ProfileModel.HasSameProfileSettings(left, right));
    }

    [Fact]
    public void PromotesSubmissionIntoCanonicalCloudProfile()
    {
        var submission = ProfileModel.SubmissionEntityFromRequest("HP", "Smart Tank 580", new ProfileRequest(
            DisplayName: "HP Smart Tank 580",
            Manufacturer: "HP",
            Model: "Smart Tank 580",
            OutputFace: "up",
            FirstPassParity: "even",
            SecondPassParity: "odd",
            FirstPassOrder: "normal",
            EvenPagesOrder: "reverse",
            ReloadMethod: "same_stack",
            Confidence: 80,
            Votes: 1,
            CreatedAt: "2026-06-14T00:00:00.000Z"), "submission-1", DateTimeOffset.Parse("2026-06-14T00:00:00.000Z"));

        var canonical = ProfileModel.PromoteSubmissionToCanonical(submission, votes: 2, DateTimeOffset.Parse("2026-06-16T00:00:00.000Z"));

        Assert.Equal("hp", canonical.PartitionKey);
        Assert.Equal("hp:smart tank 580", canonical.RowKey);
        Assert.Equal("cloud", canonical["source"]);
        Assert.Equal(2, canonical["votes"]);
    }

    [Fact]
    public void IncrementsVotesForMatchingCanonicalProfile()
    {
        var canonical = ProfileModel.EntityFromRequest("HP", "Smart Tank 580", new ProfileRequest(
            DisplayName: "HP Smart Tank 580",
            Manufacturer: "HP",
            Model: "Smart Tank 580",
            OutputFace: "up",
            FirstPassParity: "even",
            SecondPassParity: "odd",
            FirstPassOrder: "normal",
            EvenPagesOrder: "reverse",
            ReloadMethod: "same_stack",
            Confidence: 98,
            Votes: 2,
            CreatedAt: "2026-06-14T00:00:00.000Z"), DateTimeOffset.Parse("2026-06-14T00:00:00.000Z"));

        var updated = ProfileModel.IncrementCanonicalVotes(canonical, DateTimeOffset.Parse("2026-06-16T00:00:00.000Z"));

        Assert.Equal(3, updated["votes"]);
        Assert.Equal("2026-06-16T00:00:00.0000000+00:00", updated["updatedAt"]);
    }

    [Fact]
    public void NormalizesUnderscoredDriverNamesIntoStableKeys()
    {
        Assert.Equal("hp smart tank 5100 series", ProfileModel.NormalizeManufacturer("HP_Smart_Tank_5100_series"));
        Assert.Equal("hp smart tank 5100 series", ProfileModel.NormalizeModel("HP_Smart_Tank_5100_series"));
    }
}
