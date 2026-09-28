# Original creature delivery in REAL33D

Source: [3DTIBIA ARTE-CRIATURAS](https://github.com/leodavidsoto/3DTIBIA/tree/37a758df0fab94080ea7b8128db5321f5225618c/arte/criaturas).
The source manifest pins 16 original GLBs by Git blob and SHA256. They cover
15 creature forms and Rookie outfit 128, not the complete outfit catalog.
The separately found monsters3DISH catalog contains 156 sprite outfit references,
not 156 finished GLBs. No model substitution by NPC name is permitted.

## Reproduce

Prerequisites: UE 5.8, built REAL33DEditor, PythonScriptPlugin. Original V08
world assets and inventory icons remain the existing independent dependencies;
this import creates only the separate BrotherCreatures folder.

Supply a folder containing the unchanged `delivery_file` names in source.json,
or prepare that flat folder from a checkout pinned to the source commit:

```powershell
$spec = Get-Content visual/qa/brother_creatures/source.json -Raw | ConvertFrom-Json
$sourceCheckout = '<pinned-3dtibia-checkout>'
$flatDelivery = Join-Path (Get-Location) 'build/brother-creature-delivery'
New-Item -ItemType Directory -Force -Path $flatDelivery | Out-Null
foreach ($row in $spec.entries) {
    Copy-Item -LiteralPath (Join-Path $sourceCheckout $row.source_path) `
        -Destination (Join-Path $flatDelivery $row.delivery_file)
}
```

Run UnrealEditor-Cmd.exe with the REAL33D.uproject path and:

```text
-EnablePlugins=PythonScriptPlugin -run=PythonScript
-script=<repo>/visual/tools/import_brother_creatures_unreal.py
-brother-creatures-source=<flatDelivery>
-unattended -nop4 -nosplash -nullrhi -DDC-ForceMemoryCache -NoSaveConfig -stdout
```

The importer verifies each SHA before import, retains every mesh part/material,
and checks shared skeletons and four animations. Generated content is ignored
at unreal/REAL33D/Content/Experimental/BrotherCreatures; runtime.json records its
actual paths. Existing generated parts are reused, never overwritten silently.
An import PASS proves these checks only. It does not certify live appearance.

Launch the normal authoritative client with:

```text
-real33d-creature-catalog=<repo>/visual/qa/brother_creatures/runtime.json
```

Actual decoded Fusion32 outfits select models. Idle/walk follows confirmed
placement. Attack/death sequences are imported without invented triggers.
Unmatched or disguised outfits keep the previous explicit placeholder.

## Observed run

Original delivery import PASS 16/16. Fresh REAL33D loaded outfit 128 for Test
Player A and Seymour, eight parts each, 104 Unreal units high. Dixi outfit 136
has no matching delivery and remains a placeholder. Operator saw the models,
reported defects, then accepted them as they are for now. Full art certification
and complete monster/NPC coverage are not claimed.
