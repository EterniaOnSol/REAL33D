using System.IO;
using UnrealBuildTool;

// REAL33D links Protocol772Core as a prebuilt static library rather than
// recompiling its sources here. The protocol has exactly one implementation,
// in clientcore/, and this module is only its presentation consumer.
//
// Build the library first:
//   tests\build_clientcore_windows.cmd
public class REAL33D : ModuleRules
{
	public REAL33D(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		// UE 5.8 refuses to build a module at C++17. Protocol772Core's sources
		// stay C++17 and are still compiled and tested at C++17 for
		// portability; the archive linked here is a second compilation of the
		// same sources at C++20, so both sides of this link agree on the
		// standard library. See tests\build_clientcore_windows.cmd.
		CppStandard = CppStandardVersion.Cpp20;

		// Slate rather than UMG: the chat area is built in C++ like every other
		// part of this project's presentation, so it carries no .uasset and no
		// binary input asset. See SReal33DChatPanel and Real33DPlayerController.
		PublicDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject", "Engine", "InputCore", "Slate", "SlateCore"
		});

		// clientcore/ lives four directories up from Source/REAL33D.
		string RepoRoot = Path.GetFullPath(Path.Combine(ModuleDirectory, "..", "..", "..", ".."));
		string ClientCore = Path.Combine(RepoRoot, "clientcore");
		string CoreLib = Path.Combine(RepoRoot, "build", "clientcore-windows", "protocol772core.lib");

		PublicIncludePaths.Add(Path.Combine(ClientCore, "include"));

		if (!File.Exists(CoreLib))
		{
			throw new BuildException(
				"Protocol772Core static library not found at " + CoreLib +
				". Run tests\\build_clientcore_windows.cmd from an x64 Native Tools " +
				"Command Prompt before building REAL33D.");
		}
		PublicAdditionalLibraries.Add(CoreLib);

		// The same libcrypto the static library was compiled against. Using the
		// engine's own copy keeps one OpenSSL in the process.
		AddEngineThirdPartyPrivateStaticDependencies(Target, "OpenSSL");

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PublicSystemLibraries.AddRange(new string[] {
				"ws2_32.lib", "bcrypt.lib", "crypt32.lib"
			});
		}
	}
}
