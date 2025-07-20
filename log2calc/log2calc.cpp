// This program is used to generate the fixed-point log2 tables
// The basic idea is that given a value of the form:
// 1.aabbccdd
// ... we can compute a table of multipliers for each byte that will reduce that byte to 0,
// and increase the log2 by a corresponding amount.

#include <stdint.h>
#include <stdio.h>

#include <cmath>
#include <utility>

#include <intrin.h>

uint64_t divideMantissaByWhole(uint64_t largeMantissa, uint64_t whole)
{
	uint64_t remainder = 0;
	return _udiv128(1, largeMantissa, whole, &remainder);
}

int main(int argc, const char **argv)
{
	const uint64_t oneFixed32 = 0x100000000;

	double u32exp = static_cast<double>(0x10000000) * 16.0;
	double inv_u32exp = 1.0 / u32exp;

	int bitStepping = 8;
	int maxBitPos = 16;
	for (int bitPos = 0; bitPos < maxBitPos; bitPos += bitStepping)
	{
		printf("For bit pos %i\n", bitPos);

		int numEntries = (1 << bitStepping);
		int nextBitPos = bitPos + bitStepping;

		int numLowerBits = 32 - nextBitPos;

		const uint32_t lowBitFill32 = (static_cast<uint32_t>(1) << numLowerBits) - 1;

		for (int entry = 1; entry < numEntries; entry++)
		{
			const uint64_t maxForEntryInclMantissa = (static_cast<uint32_t>(entry) << numLowerBits) + lowBitFill32;
			const uint64_t maxAfterAdjustmentExcl = oneFixed32 + lowBitFill32 + 1;
			//double multiplier = static_cast<double>(maxAfterAdjustment) / static_cast<double>(maxMantissaExcl32);
			//Multiplier = maxAfterAdjustment / maxForEntry

			// maxForEntryIncl * multiplier / bitShift < maxAfterAdjustmentExcl
			// maxForEntryIncl * multiplier < maxAfterAdjustmentExcl * bitShift
			// multiplier < maxAfterAdjustmentExcl * bitShift / maxForEntryIncl
			// multiplier < (lowBitFill32 + 1 + oneFixed32) * bitShift / maxForEntryIncl
			// multiplier < ((lowBitFill32 + 1) * bitShift + oneFixed32 * bitShift) / maxForEntryIncl

			// (oneFixed32 * bitShift) is always 1 << 64
			// So, we should just use (lowBitFill32 + 1) * bitShift
			const uint64_t quotient = static_cast<uint64_t>(lowBitFill32 + 1) << 32;
			uint64_t multiplier = divideMantissaByWhole(quotient, maxForEntryInclMantissa + oneFixed32);

			// Multiplier is now either exactly the right value or 1+the right value

			// Multiplication process, in theory:
			// High 32 bits: 1   Low 32 bits: maxForEntryInclMantissa
			// Multiply by multiplier (32-bit number)

			uint64_t wholePart = static_cast<uint64_t>(multiplier) << 32;
			uint64_t mantissaPart = multiplier * maxForEntryInclMantissa;

			uint64_t result = wholePart + mantissaPart;

			result /= static_cast<uint64_t>(1) << 32;

			if (result > lowBitFill32)
			{
				multiplier--;
			}

			double floatMultiplier = static_cast<double>(multiplier) * inv_u32exp;
			double log2FloatMultiplier = -log2(floatMultiplier);

			uint32_t log2UIntMultiplier = static_cast<uint32_t>(floor(log2FloatMultiplier * u32exp));

			//printf("%i %i (%x) -> %x (%g) log2 = %g / %x\n", bitPos, entry, entry, static_cast<unsigned int>(multiplier), floatMultiplier, log2FloatMultiplier, log2UIntMultiplier);
			if (bitPos + bitStepping == maxBitPos)
			{
				printf("0x%xu,\n", log2UIntMultiplier);
			}
			else
			{
				printf("{ 0x%xu, 0x%xu },\n", static_cast<unsigned int>(multiplier), log2UIntMultiplier);
			}
		}
	}

	return 0;
}

