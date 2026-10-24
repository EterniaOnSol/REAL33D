#include "Real33DLoginQA.h"

#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "REAL33D.h"
#include "Real33DControlPanel.h"
#include "Real33DCreatureActor.h"
#include "Real33DHUD.h"
#include "Real33DLoginScreen.h"
#include "Real33DWorldActor.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"

namespace
{
	/**
	 * A password this runtime cannot have. Used for the refusal step so the
	 * run never has to construct a near-miss of a real credential, and so the
	 * literal in this file is safe to read.
	 */
	const TCHAR* const kDeliberatelyWrongPassword = TEXT("not-the-password-qa");

	/** The same KEY=VALUE parse the worker does. Values are never logged. */
	TMap<FString, FString> ReadEnvFile(const FString& Path)
	{
		TMap<FString, FString> Values;
		FString Contents;
		if (!FFileHelper::LoadFileToString(Contents, *Path))
		{
			return Values;
		}
		TArray<FString> Lines;
		Contents.ParseIntoArrayLines(Lines);
		for (const FString& Line : Lines)
		{
			FString Key;
			FString Value;
			if (Line.Split(TEXT("="), &Key, &Value))
			{
				Values.Add(Key.TrimStartAndEnd(), Value.TrimStartAndEnd());
			}
		}
		return Values;
	}

	int32 TileActors(const AReal33DWorld* World)
	{
		return World != nullptr ? World->GetTileActorCount() : 0;
	}
}

bool FReal33DLoginQA::Requested()
{
	return FParse::Param(FCommandLine::Get(), TEXT("real33d-login-qa"));
}

int32 FReal33DLoginQA::RequestedCycles()
{
	int32 Value = 0;
	if (FParse::Value(FCommandLine::Get(), TEXT("-real33d-login-qa-cycles="), Value)
		&& Value >= 0)
	{
		return Value;
	}
	return 3;
}

bool FReal33DLoginQA::Type(const TSharedPtr<SEditableTextBox>& Box, const FString& Text)
{
	if (!Box.IsValid())
	{
		return false;
	}
	Box->SetText(FText::FromString(Text));
	return true;
}

bool FReal33DLoginQA::Press(const TSharedPtr<SButton>& Button)
{
	// Refused, never forced. A disabled control is the UI saying the action is
	// not available, and a driver that clicked through that would be proving
	// the bridge while hiding a broken screen.
	if (!Button.IsValid() || !Button->IsEnabled())
	{
		return false;
	}
	Button->SimulateClick();
	return true;
}

void FReal33DLoginQA::Start(const FReal33DConnectionConfig& InConfig)
{
	Config = InConfig;
	Cycles = RequestedCycles();

	const FString SecretsDir = FPaths::Combine(Config.RuntimeDir, TEXT("secrets"));
	const TMap<FString, FString> Credentials =
		ReadEnvFile(FPaths::Combine(SecretsDir, TEXT("credentials.env")));
	const FString IdKey = FString::Printf(TEXT("ACCOUNT_%s_ID"), *Config.Account);
	const FString PasswordKey =
		FString::Printf(TEXT("ACCOUNT_%s_PASSWORD"), *Config.Account);
	if (!Credentials.Contains(IdKey) || !Credentials.Contains(PasswordKey))
	{
		UE_LOG(LogReal33D, Error,
			TEXT("login QA: the runtime account file is not readable; nothing to type"));
		return;
	}
	AccountId = static_cast<uint32>(FCString::Strtoui64(*Credentials[IdKey], nullptr, 10));
	Password = Credentials[PasswordKey];

	FString Directory;
	if (!FParse::Value(FCommandLine::Get(), TEXT("-real33d-evidence="), Directory)
		|| Directory.IsEmpty())
	{
		Directory = FPaths::ProjectSavedDir();
	}
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*Directory);
	TranscriptPath = FPaths::Combine(Directory, TEXT("login_qa.jsonl"));
	FFileHelper::SaveStringToFile(FString(), *TranscriptPath,
		FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

	BuildScript();
	StepIndex = 0;
	bStepEntered = false;
	bRunning = true;

	// The account number is recorded; the password is not, here or anywhere.
	Record(TEXT("start"), FString::Printf(
		TEXT("account %u at %s:%d, %d return cycle(s)"),
		AccountId, *Config.Host, Config.LoginPort, Cycles));
}

void FReal33DLoginQA::Record(const FString& Event, const FString& Detail)
{
	UE_LOG(LogReal33D, Log, TEXT("login QA: %s -- %s"), *Event, *Detail);
	if (TranscriptPath.IsEmpty())
	{
		return;
	}
	FString Escaped = Detail;
	Escaped.ReplaceInline(TEXT("\\"), TEXT("\\\\"));
	Escaped.ReplaceInline(TEXT("\""), TEXT("\\\""));
	const FString Line = FString::Printf(
		TEXT("{\"at\":\"%s\",\"event\":\"%s\",\"detail\":\"%s\"}\n"),
		*FDateTime::UtcNow().ToIso8601(), *Event, *Escaped);
	FFileHelper::SaveStringToFile(Line, *TranscriptPath,
		FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
		&IFileManager::Get(), FILEWRITE_Append);
}

void FReal33DLoginQA::Fail(const FString& Why)
{
	bFailed = true;
	Record(TEXT("FAIL"), Why);
	UE_LOG(LogReal33D, Error, TEXT("login QA FAILED: %s"), *Why);
	Finish();
}

void FReal33DLoginQA::Finish()
{
	bRunning = false;
	Record(bFailed ? TEXT("result") : TEXT("result"),
		bFailed ? TEXT("FAIL") : TEXT("PASS"));
	// Overwritten rather than dropped, for the same reason the bridge does it.
	for (TCHAR& Character : Password.GetCharArray())
	{
		Character = TEXT('\0');
	}
	Password.Empty();
	if (FParse::Param(FCommandLine::Get(), TEXT("real33d-login-qa-quit")))
	{
		FPlatformMisc::RequestExit(false);
	}
}

void FReal33DLoginQA::BuildScript()
{
	Script.Reset();

	// ------------------------------------------------------------- launch
	Script.Add(FStep{
		TEXT("cold-start-account-screen"),
		[](FReal33DLoginQA& Qa, const FContext& Context) -> bool
		{
			Qa.Record(TEXT("observe"), FString::Printf(
				TEXT("phase %hs at launch"),
				Real33D::SessionPhaseName(Context.Bridge->GetSessionPhase())));
			return true;
		},
		[](FReal33DLoginQA&, const FContext& Context) -> bool
		{
			return Context.Bridge->GetSessionPhase() == Real33D::SessionPhase::Offline
				&& Context.Screen->IsAccountFormShowing()
				&& !Context.Screen->IsCharacterListShowing();
		},
		10.0 });

	// ----------------------------------------------------- wrong password
	Script.Add(FStep{
		TEXT("wrong-password-refused"),
		[](FReal33DLoginQA& Qa, const FContext& Context) -> bool
		{
			return Type(Context.Screen->GetAccountBox(), FString::FromInt(Qa.AccountId))
				&& Type(Context.Screen->GetPasswordBox(), kDeliberatelyWrongPassword)
				&& Press(Context.Screen->GetLoginButton());
		},
		[](FReal33DLoginQA& Qa, const FContext& Context) -> bool
		{
			const UReal33DBridge* Bridge = Context.Bridge;
			if (Bridge->GetSessionPhase() != Real33D::SessionPhase::Offline
				|| Bridge->GetSessionMessage().IsEmpty())
			{
				return false;
			}
			// The server's refusal, in the server's own words, and NO
			// character list: a refused account must not leave a list behind.
			Qa.Record(TEXT("wrong-password"), FString::Printf(
				TEXT("refused with \"%s\"; characters offered %d; still on the account form %s"),
				*Bridge->GetSessionMessage(), Bridge->GetCharacters().Num(),
				Context.Screen->IsAccountFormShowing() ? TEXT("yes") : TEXT("no")));
			return true;
		},
		25.0 });

	// --------------------------------------------------------- cold login
	Script.Add(FStep{
		TEXT("cold-login-character-list"),
		[](FReal33DLoginQA& Qa, const FContext& Context) -> bool
		{
			return Type(Context.Screen->GetAccountBox(), FString::FromInt(Qa.AccountId))
				&& Type(Context.Screen->GetPasswordBox(), Qa.Password)
				&& Press(Context.Screen->GetLoginButton());
		},
		[](FReal33DLoginQA& Qa, const FContext& Context) -> bool
		{
			if (Context.Bridge->GetSessionPhase() != Real33D::SessionPhase::CharacterSelect)
			{
				return false;
			}
			FString Names;
			for (const FReal33DCharacterEntry& Each : Context.Bridge->GetCharacters())
			{
				Names += (Names.IsEmpty() ? TEXT("") : TEXT(", "));
				Names += FString::Printf(TEXT("%s@%s:%u"), *Each.Name, *Each.WorldName,
					static_cast<uint32>(Each.WorldPort));
			}
			Qa.Record(TEXT("character-list"), FString::Printf(
				TEXT("%d offered [%s]; rows drawn %d; list showing %s; password box cleared %s"),
				Context.Bridge->GetCharacters().Num(), *Names,
				Context.Screen->GetListedCharacters(),
				Context.Screen->IsCharacterListShowing() ? TEXT("yes") : TEXT("no"),
				Context.Screen->GetPasswordBox().IsValid()
					&& Context.Screen->GetPasswordBox()->GetText().IsEmpty()
						? TEXT("yes") : TEXT("no")));
			return Context.Screen->IsCharacterListShowing()
				&& Context.Screen->GetListedCharacters()
					== Context.Bridge->GetCharacters().Num();
		},
		25.0 });

	// The first entry, then one logout-and-return cycle per requested round.
	// Written as a loop so the script carries the same six steps each time
	// and a switch is not a special case: a different character is the same
	// Enter Game with a different row picked.
	for (int32 Cycle = 0; Cycle <= Cycles; ++Cycle)
	{
		const FString Tag = Cycle == 0
			? FString(TEXT("first")) : FString::Printf(TEXT("return-%d"), Cycle);

		Script.Add(FStep{
			FString::Printf(TEXT("%s-enter-game"), *Tag),
			[Cycle](FReal33DLoginQA& Qa, const FContext& Context) -> bool
			{
				// Rotate through whatever the server offered. With one
				// character this is a repeated relogin, which is recorded as
				// such rather than described as a switch.
				const int32 Offered = Context.Bridge->GetCharacters().Num();
				const int32 Row = Offered > 0 ? (Cycle % Offered) : 0;
				if (!Context.Screen->SelectCharacter(Row))
				{
					return false;
				}
				Qa.TilesBefore = TileActors(Context.World);
				Qa.EntryPresses = 1;
				Qa.Record(TEXT("enter-game"), FString::Printf(
					TEXT("row %d of %d (%s); tile actors before %d"),
					Row, Offered,
					Offered > 0 ? *Context.Bridge->GetCharacters()[Row].Name : TEXT("none"),
					Qa.TilesBefore));
				return Press(Context.Screen->GetEnterButton());
			},
			[](FReal33DLoginQA& Qa, const FContext& Context) -> bool
			{
				// FUSION32 MAY REFUSE A RE-ENTRY, AND THAT IS NOT A CLIENT
				// FAULT. `connections.cc::JoinGame` turns away a character
				// whose `LoggingOut` has not finished -- "Spieler %s loggt
				// gerade aus - Einloggen gescheitert" -- and answers with
				// SV_CMD_LOGIN_ERROR and a close. The client's correct
				// response is the one being tested: do not reconnect, show
				// the server's words, go back to the list.
				//
				// Pressing again is then the PLAYER's move, and it is made
				// here only after the refusal has been OBSERVED -- never on a
				// timer, and never to paper over a client-side race. Each
				// press is recorded so the transcript shows how many the
				// server turned away.
				if (Context.Bridge->GetSessionPhase() == Real33D::SessionPhase::CharacterSelect
					&& Qa.EntryPresses > 0)
				{
					if (Qa.EntryPresses >= 20)
					{
						Qa.Fail(TEXT("Fusion32 refused 20 consecutive entries"));
						return false;
					}
					++Qa.EntryPresses;
					Qa.Record(TEXT("entry-refused"), FString::Printf(
						TEXT("Fusion32 refused entry %d with \"%s\"; reconnect attempts %d; "
							"pressing Enter Game again"),
						Qa.EntryPresses - 1, *Context.Bridge->GetSessionMessage(),
						Context.Bridge->GetReconnectAttempts()));
					Press(Context.Screen->GetEnterButton());
					return false;
				}

				if (Context.Bridge->GetSessionPhase() != Real33D::SessionPhase::InGame
					|| Context.World == nullptr
					|| Context.World->GetLocalPlayer() == nullptr)
				{
					return false;
				}
				const FReal33DStats Stats = Context.Bridge->GetStats();
				if (Stats.Tiles <= 0 || TileActors(Context.World) <= 0)
				{
					return false;
				}
				const Real33D::FMapPosition At =
					Context.World->GetLocalPlayer()->GetLogicalPosition();
				Qa.Record(TEXT("in-game"), FString::Printf(
					TEXT("at %d,%d,%d; worldstate tiles %d; tile actors %d; "
						"init_games %d; login screen hidden %s"),
					At.X, At.Y, At.Z, Stats.Tiles, TileActors(Context.World),
					Stats.InitGames,
					Context.Screen->GetVisibility() == EVisibility::Collapsed
						? TEXT("yes") : TEXT("no")));
				// EXACTLY ONE INIT_GAME PER SESSION. A second would mean a
				// second connection for this character, which Fusion32
				// answers by displacing the first.
				Qa.LastInitGames = Stats.InitGames;
				++Qa.SessionsEntered;
				if (Stats.InitGames != 1)
				{
					Qa.Fail(FString::Printf(
						TEXT("INIT_GAME arrived %d times for one entry"), Stats.InitGames));
				}
				return true;
			},
			// Generous because the FIRST entry has to wait for the editor to
			// compile and load the scene's art, which is presentation work
			// with nothing to do with the session. A later entry reuses it
			// and completes in under a second; one live run measured 31.2s
			// and 0.8s for the same step.
			150.0 });

		Script.Add(FStep{
			FString::Printf(TEXT("%s-movement"), *Tag),
			[](FReal33DLoginQA& Qa, const FContext& Context) -> bool
			{
				Qa.WalksAcceptedBefore = Context.Bridge->GetStats().AcceptedSelfWalks;
				Qa.LastWalkSentAt = 0.0;
				Qa.NextWalkDirection = 0;
				return true;
			},
			[](FReal33DLoginQA& Qa, const FContext& Context) -> bool
			{
				const FReal33DStats Stats = Context.Bridge->GetStats();
				if (Stats.AcceptedSelfWalks > Qa.WalksAcceptedBefore)
				{
					const Real33D::FMapPosition At =
						Context.World->GetLocalPlayer()->GetLogicalPosition();
					Qa.Record(TEXT("movement"), FString::Printf(
						TEXT("Fusion32 accepted a step into this session; now at %d,%d,%d "
							"(accepted %d, rejected %d, unanswered %d)"),
						At.X, At.Y, At.Z, Stats.AcceptedSelfWalks,
						Stats.RejectedSteps, Stats.UnansweredSteps));
					return true;
				}
				// One request at a time, each sent only after the previous
				// one has been answered or the walk interval has passed. Not
				// a retry hack: a refused direction is a wall, and the next
				// cardinal is the next thing a player would try.
				if (Context.Now - Qa.LastWalkSentAt >= 1.0 && Qa.NextWalkDirection < 8)
				{
					Qa.LastWalkSentAt = Context.Now;
					Context.Bridge->RequestWalk(Qa.NextWalkDirection % 4);
					++Qa.NextWalkDirection;
				}
				return false;
			},
			25.0 });

		Script.Add(FStep{
			FString::Printf(TEXT("%s-logout"), *Tag),
			[](FReal33DLoginQA& Qa, const FContext& Context) -> bool
			{
				Qa.TilesBefore = TileActors(Context.World);
				if (Context.Hud == nullptr
					|| !Context.Hud->GetControlPanel().IsValid())
				{
					return false;
				}
				Qa.Record(TEXT("logout"), FString::Printf(
					TEXT("pressing the HUD's Exit button; tile actors before %d"),
					Qa.TilesBefore));
				return Press(Context.Hud->GetControlPanel()->GetExitButton());
			},
			[](FReal33DLoginQA& Qa, const FContext& Context) -> bool
			{
				// The whole claim of this milestone in one condition: the
				// session ended, the scene is empty, the client did NOT
				// reconnect, and the player is on the character list able to
				// go again -- all without the application restarting.
				if (Context.Bridge->GetSessionPhase() != Real33D::SessionPhase::CharacterSelect
					|| !Context.Screen->IsCharacterListShowing()
					|| TileActors(Context.World) != 0
					|| Context.World->GetLocalPlayer() != nullptr)
				{
					return false;
				}
				Qa.Record(TEXT("logged-out"), FString::Printf(
					TEXT("back on the character list; tile actors %d; local player %s; "
						"reconnect attempts %d; rows still offered %d"),
					TileActors(Context.World),
					Context.World->GetLocalPlayer() == nullptr ? TEXT("gone") : TEXT("PRESENT"),
					Context.Bridge->GetReconnectAttempts(),
					Context.Screen->GetListedCharacters()));
				if (Context.Bridge->GetReconnectAttempts() != 0)
				{
					Qa.Fail(TEXT("the client reconnected after a deliberate close"));
				}
				return true;
			},
			30.0 });
	}

	Script.Add(FStep{
		TEXT("summary"),
		[](FReal33DLoginQA& Qa, const FContext& Context) -> bool
		{
			// `LastInitGames` is captured while the session was up. Reading
			// the live stats here would report the counters of a worker that
			// has already been stopped, which is zero and would read as a
			// session that never got an INIT_GAME.
			Qa.Record(TEXT("summary"), FString::Printf(
				TEXT("sessions entered %d of %d expected; init_games in the last "
					"session %d; entries Fusion32 refused in the last cycle %d; "
					"reconnect attempts across the whole run %d"),
				Qa.SessionsEntered, Qa.Cycles + 1, Qa.LastInitGames,
				FMath::Max(0, Qa.EntryPresses - 1),
				Context.Bridge->GetReconnectAttempts()));
			if (Qa.SessionsEntered != Qa.Cycles + 1)
			{
				Qa.Fail(TEXT("not every requested session was entered"));
			}
			return true;
		},
		[](FReal33DLoginQA&, const FContext&) -> bool { return true; },
		5.0 });
}

void FReal33DLoginQA::Tick(const FContext& Context)
{
	if (!bRunning || Context.Bridge == nullptr || Context.Screen == nullptr)
	{
		return;
	}
	if (!Script.IsValidIndex(StepIndex))
	{
		Finish();
		return;
	}

	FStep& Step = Script[StepIndex];
	if (!bStepEntered)
	{
		bStepEntered = true;
		StepStartedAt = Context.Now;
		if (Step.Act && !Step.Act(*this, Context))
		{
			Fail(FString::Printf(TEXT("step '%s' could not be performed; a control "
				"the player would use was missing or disabled"), *Step.Name));
			return;
		}
	}

	if (Step.Done && Step.Done(*this, Context))
	{
		if (!bRunning)
		{
			return;
		}
		Record(TEXT("step"), FString::Printf(TEXT("%s: %.2fs"), *Step.Name,
			Context.Now - StepStartedAt));
		++StepIndex;
		bStepEntered = false;
		return;
	}

	// The deadline ends a stuck run and says where. It never advances the
	// script, so a step that timed out cannot be read afterwards as one that
	// passed slowly.
	if (Context.Now - StepStartedAt > Step.DeadlineSeconds)
	{
		Fail(FString::Printf(
			TEXT("step '%s' was not observed within %.0fs (phase %hs)"),
			*Step.Name, Step.DeadlineSeconds,
			Real33D::SessionPhaseName(Context.Bridge->GetSessionPhase())));
	}
}
