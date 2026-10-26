#pragma once

#include <cstdint>

// What a login log line is allowed to say about an account.
//
// THIS EXISTS BECAUSE THE ACCOUNT NUMBER USED TO BE LOGGED. Through
// UNREAL-LOGIN-CHARACTER-SELECT-UI-001 only the password was treated as a
// credential: `UReal33DBridge::BeginAccountLogin` and `FReal33DLoginQA::Start`
// each put the raw number in a format string, with a comment saying so, and the
// number therefore reached `REAL33D.log` and the QA transcript -- a file that is
// then packaged as evidence. The independent certification of that milestone
// failed on exactly that: four occurrences of the runtime account id in the
// retained raw text, zero passwords.
//
// An account number is half of a credential. It is the field Fusion32's
// `SendLoginError` answers "Accountnumber or password is not correct." about,
// it is typed into the same form, and it is the long-lived half -- a password
// can be changed, an account number cannot. So it is treated here exactly as
// the password is: it never enters a log line, an event, a transcript or an
// evidence file.
//
// WHAT REPLACES IT. The diagnostics that were actually useful, none of which
// needs the number:
//
//   * WHERE the account came from -- the player's typed form, or the sanitized
//     runtime account file that every `-real33d-account=` QA script selects.
//     This is the distinction that matters when reading a failed login, and the
//     old line did not even carry it.
//   * WHETHER two logins in one run used the same account, through `AccountTag`
//     -- a short label mixed from the number AND a salt drawn fresh for each
//     process. Within a run, the same account always tags the same, so a
//     relogin can be followed; across runs nothing correlates, and with the
//     salt never written down the label cannot be turned back into a number.
//   * WHICH attempt it was, kept in the CALLER's line rather than in the
//     account descriptor, because an attempt ordinal is a fact about the
//     session and not about the account.
//
// THE INVARIANT, stated so it can be executed: every string this header
// produces about an account is free of decimal digits. The tag alphabet has no
// digit in it and neither does any source name, so no account number of any
// length can appear in one no matter what it hashes to. `CarriesNoDecimalDigit`
// is that invariant as a predicate; tests/real33d_credential_log_tests.cpp and
// REAL33D.Presentation.CredentialLog both assert it.
//
// Free of Unreal types on purpose, like `Real33DSessionFlow.h` and
// `Real33DReconnectPolicy.h`: the native suite compiles it with a plain C++
// toolchain, so the rule is provable without a renderer, a server or a log file.
namespace Real33D
{

/**
 * The tag alphabet: sixteen consonants, so a tag is four characters of base-16
 * written WITHOUT the digits 0-9.
 *
 * That is the whole reason it is not hex. A hex tag could legitimately come out
 * as "7720", and an account number that happened to match its own tag would be
 * a leak that no amount of hashing prevented and that nothing would notice --
 * it would read as an opaque label. With no digit in the alphabet the question
 * cannot arise. The vowels are left out too, so a tag cannot spell a word and
 * be mistaken for a name.
 */
inline constexpr char kAccountTagAlphabet[] = "bcdfghjkmnpqrstv";

/** Characters in a tag. Four of sixteen symbols: 65536 labels. */
inline constexpr int kAccountTagLength = 4;

/** Where the account for a login attempt came from. Never a credential. */
enum class AccountSource : std::uint8_t
{
	/** The player typed it into the account form. */
	TypedForm,
	/**
	 * The sanitized runtime account file named it, which is what
	 * `-real33d-account=A|B` selects and what every existing QA script uses.
	 * In this mode no credential reaches the game thread at all.
	 */
	RuntimeFile,
};

/** The source's name, for logs and evidence. Digit-free, like everything here. */
inline const char* AccountSourceName(AccountSource Source)
{
	switch (Source)
	{
	case AccountSource::TypedForm:   return "typed";
	case AccountSource::RuntimeFile: return "runtime-file";
	}
	return "unknown";
}

/** A log-safe stand-in for an account number. */
struct AccountTag
{
	char Text[kAccountTagLength + 1] = {};
};

/**
 * splitmix64's finaliser over a value and a salt.
 *
 * The salt is what makes everything below one-way in practice rather than only
 * in form. Fusion32 account numbers live in a space a few million wide and the
 * rest of any preimage a client could build -- host, port, runtime path,
 * protocol version -- is printed in the log's own command line, so an UNSALTED
 * digest of an account number is a preimage an attacker already knows the shape
 * of and could simply enumerate. A salt that is never written into a log or an
 * evidence file leaves nothing to enumerate against.
 *
 * The value is offset by one so that account zero -- this client's own marker
 * for "use the runtime account file" -- does not simply mix the salt with
 * itself.
 */
inline std::uint64_t MixWithSalt(std::uint64_t Value, std::uint64_t Salt)
{
	std::uint64_t Z = Salt + 0x9E3779B97F4A7C15ull * (Value + 1ull);
	Z = (Z ^ (Z >> 30)) * 0xBF58476D1CE4E5B9ull;
	Z = (Z ^ (Z >> 27)) * 0x94D049BB133111EBull;
	Z ^= Z >> 31;
	return Z;
}

/**
 * A salt, mixed from whatever entropy the caller can get hold of.
 *
 * Never returns zero, so a caller can use zero to mean "not drawn yet".
 */
inline std::uint64_t MakeAccountTagSalt(std::uint64_t EntropyA, std::uint64_t EntropyB)
{
	const std::uint64_t Z = MixWithSalt(EntropyA, EntropyB);
	return Z != 0 ? Z : 0x9E3779B97F4A7C15ull;
}

/**
 * A stable, non-reversible scope for per-account data this client keeps on
 * disk -- the minimap cache being the only one.
 *
 * Full width rather than a tag, because this one names a FILE: a tag's 65536
 * labels would let two accounts collide onto one cache and quietly share a map
 * memory, which a log label can afford and a store cannot. With a salt that
 * persists beside the data, a scope stays addressable across restarts without
 * the account number being recoverable from it.
 */
inline std::uint64_t MakeAccountStorageScope(std::uint32_t AccountNumber, std::uint64_t Salt)
{
	return MixWithSalt(std::uint64_t(AccountNumber), Salt);
}

/**
 * The label this process uses for an account number.
 *
 * Stable for one salt, so two logins of one account tag alike and a relogin can
 * be followed through a log. Useless to anybody without the salt, which is
 * never written anywhere.
 */
inline AccountTag MakeAccountTag(std::uint32_t AccountNumber, std::uint64_t Salt)
{
	const std::uint64_t Z = MixWithSalt(std::uint64_t(AccountNumber), Salt);

	AccountTag Tag{};
	for (int Index = 0; Index < kAccountTagLength; ++Index)
	{
		Tag.Text[Index] = kAccountTagAlphabet[(Z >> (4 * Index)) & 0xFull];
	}
	Tag.Text[kAccountTagLength] = '\0';
	return Tag;
}

/** Everything a log line may say about an account, as one string. */
struct AccountLogText
{
	char Text[48] = {};
};

/**
 * The account descriptor a login line carries, in place of the number.
 *
 * Composed by appending fixed words rather than through a format string, and
 * that is deliberate: there is no conversion in here that could be handed an
 * account number, so the only way to log one through this function would be to
 * add a parameter for it.
 */
inline AccountLogText DescribeAccountForLog(AccountSource Source, const AccountTag& Tag)
{
	AccountLogText Out{};
	int At = 0;
	const auto Append = [&Out, &At](const char* Word)
	{
		for (int Index = 0; Word[Index] != '\0' && At + 1 < int(sizeof Out.Text); ++Index)
		{
			Out.Text[At++] = Word[Index];
		}
		Out.Text[At] = '\0';
	};
	Append(AccountSourceName(Source));
	Append(" account (tag ");
	Append(Tag.Text);
	Append(")");
	return Out;
}

/**
 * The header's invariant as a predicate: no decimal digit anywhere in `Text`.
 *
 * A string with no digit in it cannot contain an account number, a password
 * made of digits, or any other decimal credential, whatever its length. Cheap
 * enough that both test suites assert it over wide sweeps rather than over a
 * couple of examples.
 */
inline bool CarriesNoDecimalDigit(const char* Text)
{
	if (Text == nullptr)
	{
		return true;
	}
	for (int Index = 0; Text[Index] != '\0'; ++Index)
	{
		if (Text[Index] >= '0' && Text[Index] <= '9')
		{
			return false;
		}
	}
	return true;
}

} // namespace Real33D
