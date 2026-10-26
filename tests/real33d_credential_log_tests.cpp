// What a login log line is allowed to say about an account, without Unreal and
// without a log file.
//
// THE REGRESSION. Through UNREAL-LOGIN-CHARACTER-SELECT-UI-001 the client
// logged the raw account number -- `UReal33DBridge::BeginAccountLogin` and
// `FReal33DLoginQA::Start` each put it in a format string -- so it reached
// REAL33D.log and the QA transcript, and from there the packaged evidence. The
// independent certification failed on exactly that: four occurrences of the
// runtime account id, zero passwords.
//
// WHAT THIS PROVES. That the only strings the client now produces about an
// account carry NO DECIMAL DIGIT AT ALL. That is a stronger claim than "the
// test accounts do not appear in them", and it implies it: a string with no
// digit in it cannot contain an account number of any length, this runtime's or
// anybody's. The known runtime pair is swept explicitly as well, so the
// certification's own finding has a test that reproduces it as a pass.
//
// Neither this file nor the suite's output ever prints a swept value. The
// end-to-end half of the claim -- that the real values occur zero times in the
// logs and evidence a run actually produced, passwords included -- is
// tests/scan_credential_values.py, run by the harness as `credential-value-scan`
// against the runtime secrets file, which stays outside Git.

#include "Real33DCredentialLog.h"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <set>
#include <string>

namespace
{
using namespace Real33D;

constexpr AccountSource kBothSources[] = {
	AccountSource::TypedForm,
	AccountSource::RuntimeFile,
};

/**
 * The accounts scripts/server/prepare_wsl.sh writes into every sanitized
 * runtime, which are the ones the failed certification found in the log.
 *
 * Numbers, not secrets: that script is tracked and writes these two literals on
 * every preparation. The secret half of the pair is the generated password,
 * which nothing here has or needs -- see `TheDescriptorIsAClosedForm` for why
 * no password can reach a line this header builds.
 */
constexpr std::uint32_t kRuntimeAccounts[] = { 772001u, 772002u };

/** A spread of salts, so no assertion below rests on one lucky draw. */
std::uint64_t SaltAt(int Index)
{
	return MakeAccountTagSalt(0x5DEECE66Dull * std::uint64_t(Index + 1),
		0xC0FFEE00ull ^ std::uint64_t(Index));
}

void ThePredicateCanActuallySeeADigit()
{
	// First, because every assertion in this file is expressed through it. A
	// digit check that cannot match a digit is indistinguishable from a clean
	// result, which is the same reason tests/secret_check.sh self-tests its
	// PEM pattern before trusting it.
	assert(!CarriesNoDecimalDigit("772001"));
	assert(!CarriesNoDecimalDigit("typed account (tag 7)"));
	assert(!CarriesNoDecimalDigit("0"));
	assert(!CarriesNoDecimalDigit("9"));
	for (char Digit = '0'; Digit <= '9'; ++Digit)
	{
		const char Probe[] = { 'x', Digit, 'y', '\0' };
		assert(!CarriesNoDecimalDigit(Probe));
	}
	assert(CarriesNoDecimalDigit(""));
	assert(CarriesNoDecimalDigit(nullptr));
	assert(CarriesNoDecimalDigit("typed account (tag mnpq)"));
}

void TheTagAlphabetHasNoDigitInIt()
{
	// The structural reason the rest holds. A hex tag could legitimately come
	// out as "7720"; with no digit in the alphabet an account number cannot
	// appear in a tag no matter what it hashes to.
	const std::size_t Symbols = std::strlen(kAccountTagAlphabet);
	assert(Symbols == 16);
	assert(CarriesNoDecimalDigit(kAccountTagAlphabet));

	// Distinct symbols, or the tag space is smaller than it claims.
	std::set<char> Distinct;
	for (std::size_t Index = 0; Index < Symbols; ++Index)
	{
		Distinct.insert(kAccountTagAlphabet[Index]);
	}
	assert(Distinct.size() == Symbols);

	// And every tag is drawn from exactly that alphabet, at exactly that width.
	for (int Salt = 0; Salt < 32; ++Salt)
	{
		for (std::uint32_t Account = 0; Account < 4096; ++Account)
		{
			const AccountTag Tag = MakeAccountTag(Account, SaltAt(Salt));
			assert(std::strlen(Tag.Text) == std::size_t(kAccountTagLength));
			for (int Index = 0; Index < kAccountTagLength; ++Index)
			{
				assert(Distinct.count(Tag.Text[Index]) == 1);
			}
		}
	}
}

void NoSweptAccountNumberSurvivesIntoALine()
{
	// The whole account space, sampled across every decade and every width a
	// Fusion32 account can have, against many salts. Two assertions each: the
	// absolute one -- not a digit anywhere -- and the literal one the
	// certification scanner makes, that the number's own decimal text does not
	// occur in the line.
	long long Checked = 0;
	for (int Salt = 0; Salt < 16; ++Salt)
	{
		for (std::uint64_t Account = 0; Account <= 0xFFFFFFFFull;
			Account = Account < 10 ? Account + 1 : Account + (Account / 7) + 1)
		{
			const std::uint32_t Number = std::uint32_t(Account);
			const std::string Decimal = std::to_string(Number);
			for (const AccountSource Source : kBothSources)
			{
				const AccountLogText Line = DescribeAccountForLog(
					Source, MakeAccountTag(Number, SaltAt(Salt)));
				assert(CarriesNoDecimalDigit(Line.Text));
				assert(std::strstr(Line.Text, Decimal.c_str()) == nullptr);
				++Checked;
			}
		}
	}
	assert(Checked > 1000);
}

void TheKnownRuntimeAccountsNeverAppear()
{
	// The certification's own finding, as a test that passes. Both runtime
	// accounts, both sources, 512 independent salts: the id is absent from
	// every line, and so is every other decimal string.
	for (const std::uint32_t Account : kRuntimeAccounts)
	{
		const std::string Decimal = std::to_string(Account);
		for (int Salt = 0; Salt < 512; ++Salt)
		{
			for (const AccountSource Source : kBothSources)
			{
				const AccountLogText Line = DescribeAccountForLog(
					Source, MakeAccountTag(Account, SaltAt(Salt)));
				assert(std::strstr(Line.Text, Decimal.c_str()) == nullptr);
				assert(CarriesNoDecimalDigit(Line.Text));
			}
		}
	}
}

void TheDescriptorIsAClosedForm()
{
	// Why no password can reach one of these lines either: the descriptor is
	// built from exactly two inputs -- a source enum and a tag -- and its text
	// is fully determined by them. There is no conversion in it that could be
	// handed a credential, so logging one would take a new parameter rather
	// than a new format string.
	for (const AccountSource Source : kBothSources)
	{
		const AccountTag Tag = MakeAccountTag(772001u, SaltAt(3));
		const AccountLogText Line = DescribeAccountForLog(Source, Tag);
		const std::string Expected = std::string(AccountSourceName(Source))
			+ " account (tag " + Tag.Text + ")";
		assert(Expected == Line.Text);
		assert(CarriesNoDecimalDigit(AccountSourceName(Source)));
	}

	// Distinct, digit-free source names, because the source is the diagnostic
	// that replaced the number and two sources reading alike would lose it.
	assert(std::strcmp(AccountSourceName(AccountSource::TypedForm),
		AccountSourceName(AccountSource::RuntimeFile)) != 0);
}

void ATagFollowsOneAccountThroughOneRunAndNoFurther()
{
	const std::uint64_t Salt = SaltAt(7);

	// Stable under one salt: this is the diagnostic that replaced the number.
	// A relogin of the same account tags the same, so a reader can tell "they
	// tried the same account again" from "they tried a different one".
	for (const std::uint32_t Account : kRuntimeAccounts)
	{
		const AccountTag First = MakeAccountTag(Account, Salt);
		const AccountTag Again = MakeAccountTag(Account, Salt);
		assert(std::strcmp(First.Text, Again.Text) == 0);
	}

	// And carries nothing across runs. One account under a thousand salts must
	// not keep one label, or the tag would correlate the same player's
	// evidence from different sessions -- a weaker leak, but the same kind.
	std::set<std::string> Labels;
	for (int Salt = 0; Salt < 1000; ++Salt)
	{
		Labels.insert(MakeAccountTag(kRuntimeAccounts[0], SaltAt(Salt)).Text);
	}
	assert(Labels.size() > 500);
}

void TagsStillTellTwoAccountsApart()
{
	// A label that collided constantly would be no diagnostic at all. 4096
	// consecutive accounts under one salt, against a 65536-label space: a few
	// collisions are expected and acceptable in a log, near-total collision is
	// not.
	std::set<std::string> Labels;
	for (std::uint32_t Account = 772000u; Account < 776096u; ++Account)
	{
		Labels.insert(MakeAccountTag(Account, SaltAt(11)).Text);
	}
	assert(Labels.size() > 3800);

	// The two runtime accounts differ by one and must not share a label under
	// any of a thousand salts more often than chance.
	int Collisions = 0;
	for (int Salt = 0; Salt < 1000; ++Salt)
	{
		if (std::strcmp(MakeAccountTag(kRuntimeAccounts[0], SaltAt(Salt)).Text,
				MakeAccountTag(kRuntimeAccounts[1], SaltAt(Salt)).Text) == 0)
		{
			++Collisions;
		}
	}
	assert(Collisions < 20);
}

void TheStorageScopeIsWideAndSalted()
{
	// The minimap cache's file name. It used to be a SHA1 over host, port,
	// protocol version, runtime path and the RAW account number -- a preimage
	// whose every other field the log prints in its own command line, leaving a
	// six-digit space anybody could enumerate. Salted, that enumeration has
	// nothing to run against; full width, because a file name that collided
	// would let two accounts quietly share one map memory.
	const std::uint64_t Salt = SaltAt(13);

	std::set<std::uint64_t> Scopes;
	for (std::uint32_t Account = 772000u; Account < 782000u; ++Account)
	{
		const std::uint64_t Scope = MakeAccountStorageScope(Account, Salt);
		// Not the identity, and not the number with a constant added: either
		// would be the credential itself in a different base.
		assert(Scope != std::uint64_t(Account));
		Scopes.insert(Scope);
	}
	assert(Scopes.size() == 10000);

	// Stable, or last run's cache would be unaddressable this run.
	for (const std::uint32_t Account : kRuntimeAccounts)
	{
		assert(MakeAccountStorageScope(Account, Salt)
			== MakeAccountStorageScope(Account, Salt));

		// And salt-dependent, or the salt would not be doing anything.
		int Moved = 0;
		for (int Other = 0; Other < 64; ++Other)
		{
			if (MakeAccountStorageScope(Account, SaltAt(Other))
				!= MakeAccountStorageScope(Account, Salt))
			{
				++Moved;
			}
		}
		assert(Moved >= 63);
	}
}

void ASaltIsNeverZero()
{
	// Both callers use zero to mean "not drawn yet", so a draw that came out
	// zero would be re-drawn on every line and the tag would stop being stable
	// within a run.
	for (int A = 0; A < 64; ++A)
	{
		for (int B = 0; B < 64; ++B)
		{
			assert(MakeAccountTagSalt(std::uint64_t(A), std::uint64_t(B)) != 0);
		}
	}
	assert(MakeAccountTagSalt(0, 0) != 0);

	// Account zero -- this client's own marker for "use the runtime account
	// file" -- is tagged like any other, rather than mixing a salt with itself.
	assert(std::strcmp(MakeAccountTag(0, SaltAt(1)).Text,
		MakeAccountTag(0, SaltAt(2)).Text) != 0);
	assert(CarriesNoDecimalDigit(MakeAccountTag(0, SaltAt(1)).Text));
}

} // namespace

int main()
{
	ThePredicateCanActuallySeeADigit();
	TheTagAlphabetHasNoDigitInIt();
	NoSweptAccountNumberSurvivesIntoALine();
	TheKnownRuntimeAccountsNeverAppear();
	TheDescriptorIsAClosedForm();
	ATagFollowsOneAccountThroughOneRunAndNoFurther();
	TagsStillTellTwoAccountsApart();
	TheStorageScopeIsWideAndSalted();
	ASaltIsNeverZero();

	std::cout << "real33d credential log tests: 9 cases passed\n";
	return 0;
}
