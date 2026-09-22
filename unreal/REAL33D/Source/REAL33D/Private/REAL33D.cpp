#include "REAL33D.h"

#include "Modules/ModuleManager.h"
#include "Real33DUIStyle.h"

DEFINE_LOG_CATEGORY(LogReal33D);

/**
 * The default game module, plus ownership of the Slate style.
 *
 * `FReal33DUIStyle` holds brushes backed by files on disk, so it has to be
 * built after Slate exists and torn down before Slate goes away. Module startup
 * and shutdown are exactly those two moments, which is the whole reason this
 * replaces `FDefaultGameModuleImpl` at the macro below.
 */
class FReal33DModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		FDefaultGameModuleImpl::StartupModule();
		FReal33DUIStyle::Initialize();
	}

	virtual void ShutdownModule() override
	{
		FReal33DUIStyle::Shutdown();
		FDefaultGameModuleImpl::ShutdownModule();
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FReal33DModule, REAL33D, "REAL33D");
