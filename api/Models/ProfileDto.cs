namespace Easure.PrintProfiles.Api.Models;

public sealed record ProfileDto(
    string DisplayName,
    string Manufacturer,
    string Model,
    string NormalizedKey,
    string OutputFace,
    string FirstPassParity,
    string SecondPassParity,
    string FirstPassOrder,
    string EvenPagesOrder,
    string ReloadMethod,
    int Confidence,
    int Votes,
    string Source,
    int SchemaVersion,
    string UpdatedAt);

public sealed record ProfileRequest(
    string? DisplayName,
    string? Manufacturer,
    string? Model,
    string? OutputFace,
    string? FirstPassParity,
    string? SecondPassParity,
    string? FirstPassOrder,
    string? EvenPagesOrder,
    string? ReloadMethod,
    int? Confidence,
    int? Votes,
    string? CreatedAt);
