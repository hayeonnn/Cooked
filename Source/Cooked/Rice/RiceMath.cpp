#include "Rice/RiceMath.h"

namespace
{
	/**
	 *  The three increments of a 3D low discrepancy sequence, kept at the exact
	 *  precision the Blueprint shipped with so the clump looks unchanged.
	 *
	 *  They have to be mutually independent. The first attempt used the golden
	 *  ratio for length and the golden angle for rotation, which are the same
	 *  sequence in disguise - 137.5 / 360 and 1 - 1/phi are both 0.381966 - so
	 *  length and angle moved together and every grain landed on one spiral.
	 */
	constexpr double LengthStep = 0.8191725;
	constexpr double AngleStep  = 0.6710436;
	constexpr double RadiusStep = 0.5497005;

	/** Exponent that squares off the cross section. 4 gives a shari, 2 a rugby ball. */
	constexpr double ProfileExponent = 4.0;

	/** FNV-1a over one integer, so the hash walks the array in order. */
	FORCEINLINE uint32 HashInt32(uint32 Hash, int32 Value)
	{
		const uint8* Bytes = reinterpret_cast<const uint8*>(&Value);

		for (int32 ByteIndex = 0; ByteIndex < 4; ++ByteIndex)
		{
			Hash ^= Bytes[ByteIndex];
			Hash *= 16777619u;
		}

		return Hash;
	}
}

float URiceMath::ClumpScaleFor(int32 GrainCount)
{
	if (GrainCount <= 1)
	{
		return 1.f;
	}

	return FMath::Pow(static_cast<float>(GrainCount), 1.f / 3.f);
}

FTransform URiceMath::GrainTransformFor(int32 GrainIndex, int32 GrainCount, FVector BaseExtent, FVector GrainScale)
{
	if (GrainIndex < 0)
	{
		return FTransform::Identity;
	}

	const FVector Extent = BaseExtent * ClumpScaleFor(GrainCount);
	const double Index = static_cast<double>(GrainIndex);

	// Offsetting the length sequence by 0.5 centres it; the radius sequence is
	// deliberately left unshifted so grain 0 lands exactly at the origin.
	const double U = FMath::Frac(Index * LengthStep + 0.5);
	const double Angle = FMath::Frac(Index * AngleStep) * 360.0;
	const double Rho = FMath::Sqrt(FMath::Frac(Index * RadiusStep));

	// How far along the length this grain is, as a distance from the middle.
	const double P = FMath::Abs(2.0 * U - 1.0);

	// Near the middle this stays close to 1 so the sides run straight, and it
	// only collapses at the very ends. That is the shari silhouette.
	const double Profile = FMath::Pow(FMath::Max(0.0, 1.0 - FMath::Pow(P, ProfileExponent)), 0.25);

	const double AngleRadians = FMath::DegreesToRadians(Angle);

	const FVector Offset(
		Extent.X * (2.0 * U - 1.0),
		Extent.Y * Profile * Rho * FMath::Cos(AngleRadians),
		Extent.Z * Profile * Rho * FMath::Sin(AngleRadians));

	// Grains are elongated, so leaving them all aligned reads as a lattice.
	// Three different multiples of the same angle turn the clump into a texture.
	const FRotator Rotation(Angle * 1.7, Angle * 2.3, Angle * 1.1);

	return FTransform(Rotation, Offset, GrainScale);
}

void URiceMath::GradeShari(int32 GrainCount, int32& OutScore, float& OutQuality, int32 TargetGrainCount, int32 ScorePerGrain, float QualityPenalty)
{
	if (GrainCount <= 0 || TargetGrainCount <= 0)
	{
		OutScore = 0;
		OutQuality = 0.f;
		return;
	}

	// Float divide on purpose. Two integer pins would make this integer
	// division in Blueprint and 62 grains would come out as 0 completeness,
	// which is how the score once came out as zero.
	const float Completeness = static_cast<float>(GrainCount) / static_cast<float>(TargetGrainCount);

	// Distance from the target costs the same in either direction. The floor of
	// 0.1 means a wildly wrong clump still scores something, so a bad delivery
	// reads as a bad score rather than as a bug.
	OutQuality = FMath::Clamp(1.f - FMath::Abs(1.f - Completeness) * QualityPenalty, 0.1f, 1.f);
	OutScore = FMath::RoundToInt(static_cast<float>(GrainCount) * static_cast<float>(ScorePerGrain) * OutQuality);
}

int32 URiceMath::RiceChecksum(const TArray<FVector>& Positions, float QuantizeStep)
{
	const float SafeStep = FMath::Max(QuantizeStep, UE_KINDA_SMALL_NUMBER);

	uint32 Hash = 2166136261u;	// FNV-1a offset basis

	for (const FVector& Position : Positions)
	{
		Hash = HashInt32(Hash, FMath::RoundToInt(Position.X / SafeStep));
		Hash = HashInt32(Hash, FMath::RoundToInt(Position.Y / SafeStep));
		Hash = HashInt32(Hash, FMath::RoundToInt(Position.Z / SafeStep));
	}

	return static_cast<int32>(Hash);
}

FString URiceMath::RiceChecksumLabel(const TArray<FVector>& Positions, int32 Seed, float QuantizeStep)
{
	return FString::Printf(TEXT("rice count: %d | seed: %d | checksum: %d"),
		Positions.Num(),
		Seed,
		RiceChecksum(Positions, QuantizeStep));
}
