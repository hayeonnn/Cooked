#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RiceMath.generated.h"

/**
 *  Pure math that every machine has to agree on.
 *
 *  Grain placement, shari grading and the debug checksum used to be Blueprint
 *  graphs. They are the parts that must produce identical results on the host
 *  and on every client, so they belong in one function instead of a graph that
 *  can quietly drift between machines - a graph compiles fine with a pin left
 *  unconnected, and the symptom only shows up as two screens disagreeing.
 *
 *  Nothing here touches actors, replication or the world. Feed it an index and
 *  a count and it returns the same answer everywhere, which is what lets a
 *  twenty grain clump cross the network as three integers.
 *
 *  The placement is a port of the Blueprint that shipped, not a redesign: same
 *  low discrepancy constants, same cross section profile, same rotation. The
 *  clump looks exactly as it did before.
 */
UCLASS()
class URiceMath : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/**
	 *  How much bigger the clump is than a single grain.
	 *
	 *  Cube root of the count, so volume grows with the grains: doubling them
	 *  widens the clump by about 26%. Placement, capsule size and angular
	 *  damping all scale off this one number, so it lives in one place.
	 */
	UFUNCTION(BlueprintPure, Category="Little Cooks|Rice")
	static float ClumpScaleFor(int32 GrainCount);

	/**
	 *  Where one grain sits inside its clump, and how it is turned.
	 *
	 *  X runs along the length of the shari. Y and Z are the cross section,
	 *  shaped by prof = (1 - p^4)^(1/4): near the middle that is close to 1 so
	 *  the sides stay straight, and it only falls off at the very ends, which
	 *  is what reads as a shari. An exponent of 2 instead of 4 gives a plain
	 *  ellipsoid with pointed ends - a rugby ball.
	 *
	 *  The three increments are independent by construction. Using the golden
	 *  ratio for length and the golden angle for rotation looks reasonable and
	 *  is wrong: 137.5 / 360 and 1 - 1/phi are the same number, so the two move
	 *  as one and the grains land on a spiral instead of filling the volume.
	 *
	 *  BaseExtent is the half size of the shari at one grain - (20, 6, 6) as
	 *  shipped. Y and Z being equal makes the cross section circular rather
	 *  than flattened, which is a deliberate call: it looked better, and the
	 *  profile exponent already keeps the sides straight enough to read as
	 *  shari. GrainScale is the scale of the grain mesh itself - (7, 3.5, 3.5)
	 *  here; an instanced mesh does not inherit the component override, so it
	 *  has to be passed through or the grains come out at their authored size.
	 */
	UFUNCTION(BlueprintPure, Category="Little Cooks|Rice")
	static FTransform GrainTransformFor(int32 GrainIndex, int32 GrainCount, FVector BaseExtent, FVector GrainScale);

	/**
	 *  Score for a delivered clump.
	 *
	 *  Quality peaks at TargetGrainCount and falls off at the same rate in both
	 *  directions, so overfilling costs as much as underfilling: at a target of
	 *  100, a 100 grain shari scores 1000 and a 200 grain one scores 200.
	 *
	 *  A flat score per delivery would make one grain spam optimal; a score
	 *  purely proportional to grains would remove any reason to clump. The
	 *  product blocks both. Finding the peak over several rounds is the game,
	 *  which is why the in-game gauge shows no number.
	 */
	UFUNCTION(BlueprintPure, Category="Little Cooks|Rice")
	static void GradeShari(int32 GrainCount, int32& OutScore, float& OutQuality, int32 TargetGrainCount = 100, int32 ScorePerGrain = 10, float QualityPenalty = 0.9f);

	/**
	 *  Checksum of a rice layout, for holding two screens side by side.
	 *
	 *  Order sensitive on purpose: if a line trace misses on one machine and not
	 *  the other, the indices shift, and that has to be reported as different
	 *  even though the surviving positions match - AllRice[RiceIndex] no longer
	 *  means the same grain on both machines.
	 *
	 *  Positions are snapped to QuantizeStep before hashing, so float noise in
	 *  the last bits cannot make identical layouts disagree. Both machines are
	 *  Windows x64, so the byte order this hashes over is the same on each.
	 */
	UFUNCTION(BlueprintPure, Category="Little Cooks|Rice")
	static int32 RiceChecksum(const TArray<FVector>& Positions, float QuantizeStep = 1.f);

	/** The checksum with the numbers it was taken over, ready to print on both screens. */
	UFUNCTION(BlueprintPure, Category="Little Cooks|Rice")
	static FString RiceChecksumLabel(const TArray<FVector>& Positions, int32 Seed, float QuantizeStep = 1.f);
};
