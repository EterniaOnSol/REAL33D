// Independent fixture check of the production static conversion function.
// Included temporarily at the end of Real33DBridge.cpp, then retired as evidence.
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutfitBridgeIndependentTest,
    "REAL33D.Certification.OutfitBridgeTransport",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOutfitBridgeIndependentTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    p772::OutfitDescriptor Source;
    Source.outfit_id=139;
    Source.colors={{3,17,87,132}};
    Source.object_type=2969; // Irrelevant union branch must not leak.
    const auto Ordinary=ToUnrealOutfit(Source);
    TestEqual(TEXT("outfit id preserved"),Ordinary.OutfitId,uint16(139));
    TestFalse(TEXT("ordinary outfit flag"),Ordinary.bDisguisedAsObject);
    TestEqual(TEXT("ordinary object branch zero"),Ordinary.ObjectType,uint16(0));
    for(int32 I=0;I<4;++I)TestEqual(FString::Printf(TEXT("wire colour %d preserved"),I),Ordinary.Colors[I],Source.colors[I]);
    Source.outfit_id=0;Source.disguised_as_object=true;
    const auto Disguise=ToUnrealOutfit(Source);
    TestEqual(TEXT("disguise id zero"),Disguise.OutfitId,uint16(0));
    TestTrue(TEXT("disguise flag preserved"),Disguise.bDisguisedAsObject);
    TestEqual(TEXT("disguise object type preserved"),Disguise.ObjectType,uint16(2969));
    for(int32 I=0;I<4;++I)TestEqual(FString::Printf(TEXT("absent disguise colour %d zero"),I),Disguise.Colors[I],uint8(0));
    Source.object_type=0;
    const auto Invisible=ToUnrealOutfit(Source);
    TestTrue(TEXT("invisible form retains disguise flag"),Invisible.bDisguisedAsObject);
    TestEqual(TEXT("invisible object zero retained"),Invisible.ObjectType,uint16(0));
    return true;
}
#endif
