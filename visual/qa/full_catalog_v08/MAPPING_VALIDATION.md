# V08 deterministic identity check

This is a static data check. It does not prove a live Fusion32 TypeId or a visible Unreal actor.

Checked rows: 4913; mismatches: 0.

| V08 ID / intended Fusion32 TypeId | Name | State | Source GLB | Imported Unreal asset | GLB hash | Pending warnings |
|---:|---|---|---|---|---|---|
| 100 | void | IN_REVIEW | `worklog/ORQUESTADOR/generaciones_assets/refinamiento_en_curso_v8/samples/models/00100.glb` | `/Game/Experimental/V08/ID_00100/SM_V08_00100.SM_V08_00100` | MATCH | DEGENERATE_UV_TRIANGLES, VERY_FLAT |
| 101 | earth | PENDING | `worklog/ORQUESTADOR/generaciones_assets/piloto_completo_4913/models/00101.glb` | `/Game/Experimental/V08/ID_00101/SM_V08_00101.SM_V08_00101` | MATCH | DEGENERATE_UV_TRIANGLES, PIVOT_NOT_AT_FLOOR |
| 102 | grass | RETAINED_REFERENCE | `worklog/ORQUESTADOR/generaciones_assets/piloto_completo_4913/models/00102.glb` | `/Game/Experimental/V08/ID_00102/SM_V08_00102.SM_V08_00102` | MATCH | PIVOT_NOT_AT_FLOOR |
| 360 | dirt wall | IN_REVIEW | `worklog/ORQUESTADOR/generaciones_assets/refinamiento_en_curso_v8/samples/models/00360.glb` | `/Game/Experimental/V08/ID_00360/SM_V08_00360.SM_V08_00360` | MATCH | DEGENERATE_UV_TRIANGLES, ZERO_AREA_TRIANGLES |
| 408 | wooden floor | PENDING | `worklog/ORQUESTADOR/generaciones_assets/piloto_completo_4913/models/00408.glb` | `/Game/Experimental/V08/ID_00408/SM_V08_00408.SM_V08_00408` | MATCH | DEGENERATE_UV_TRIANGLES, PIVOT_NOT_AT_FLOOR |
| 1270 | brick wall | RETAINED_REFERENCE | `worklog/ORQUESTADOR/generaciones_assets/piloto_completo_4913/models/01270.glb` | `/Game/Experimental/V08/ID_01270/SM_V08_01270.SM_V08_01270` | MATCH | DEGENERATE_UV_TRIANGLES |
| 1294 | stone wall | RETAINED_REFERENCE | `worklog/ORQUESTADOR/generaciones_assets/piloto_completo_4913/models/01294.glb` | `/Game/Experimental/V08/ID_01294/SM_V08_01294.SM_V08_01294` | MATCH | ? |
| 1295 | stone wall | IN_REVIEW | `worklog/ORQUESTADOR/generaciones_assets/refinamiento_en_curso_v8/samples/models/01295.glb` | `/Game/Experimental/V08/ID_01295/SM_V08_01295.SM_V08_01295` | MATCH | DEGENERATE_UV_TRIANGLES |
| 1301 | stone wall | PENDING | `worklog/ORQUESTADOR/generaciones_assets/piloto_completo_4913/models/01301.glb` | `/Game/Experimental/V08/ID_01301/SM_V08_01301.SM_V08_01301` | MATCH | DEGENERATE_UV_TRIANGLES, ZERO_AREA_TRIANGLES |
| 2303 | big table | NEEDS_ASSEMBLY | `worklog/ORQUESTADOR/generaciones_assets/piloto_completo_4913/models/02303.glb` | `/Game/Experimental/V08/ID_02303/SM_V08_02303.SM_V08_02303` | MATCH | DEGENERATE_UV_TRIANGLES, ZERO_AREA_TRIANGLES |
| 2328 | table | PENDING | `worklog/ORQUESTADOR/generaciones_assets/piloto_completo_4913/models/02328.glb` | `/Game/Experimental/V08/ID_02328/SM_V08_02328.SM_V08_02328` | MATCH | DEGENERATE_UV_TRIANGLES, ZERO_AREA_TRIANGLES |
| 2948 | wooden flute | REFINED | `worklog/ORQUESTADOR/generaciones_assets/refinamiento_23_instrumentos_v8/models/02948.glb` | `/Game/Experimental/V08/ID_02948/SM_V08_02948.SM_V08_02948` | MATCH | DEGENERATE_UV_TRIANGLES |
| 3497 | locker | PENDING | `worklog/ORQUESTADOR/generaciones_assets/piloto_completo_4913/models/03497.glb` | `/Game/Experimental/V08/ID_03497/SM_V08_03497.SM_V08_03497` | MATCH | DEGENERATE_UV_TRIANGLES, ZERO_AREA_TRIANGLES |
| 3498 | locker | PENDING | `worklog/ORQUESTADOR/generaciones_assets/piloto_completo_4913/models/03498.glb` | `/Game/Experimental/V08/ID_03498/SM_V08_03498.SM_V08_03498` | MATCH | DEGENERATE_UV_TRIANGLES, ZERO_AREA_TRIANGLES |
| 3499 | locker | PENDING | `worklog/ORQUESTADOR/generaciones_assets/piloto_completo_4913/models/03499.glb` | `/Game/Experimental/V08/ID_03499/SM_V08_03499.SM_V08_03499` | MATCH | DEGENERATE_UV_TRIANGLES, ZERO_AREA_TRIANGLES |
| 3500 | locker | PENDING | `worklog/ORQUESTADOR/generaciones_assets/piloto_completo_4913/models/03500.glb` | `/Game/Experimental/V08/ID_03500/SM_V08_03500.SM_V08_03500` | MATCH | DEGENERATE_UV_TRIANGLES, ZERO_AREA_TRIANGLES |
| 3501 | mailbox | RETAINED_REFERENCE | `worklog/ORQUESTADOR/generaciones_assets/piloto_completo_4913/models/03501.glb` | `/Game/Experimental/V08/ID_03501/SM_V08_03501.SM_V08_03501` | MATCH | DEGENERATE_UV_TRIANGLES, ZERO_AREA_TRIANGLES |
| 3502 | depot chest | RETAINED_REFERENCE | `worklog/ORQUESTADOR/generaciones_assets/piloto_completo_4913/models/03502.glb` | `/Game/Experimental/V08/ID_03502/SM_V08_03502.SM_V08_03502` | MATCH | DEGENERATE_UV_TRIANGLES, ZERO_AREA_TRIANGLES |
| 3508 | mailbox | PENDING | `worklog/ORQUESTADOR/generaciones_assets/piloto_completo_4913/models/03508.glb` | `/Game/Experimental/V08/ID_03508/SM_V08_03508.SM_V08_03508` | MATCH | DEGENERATE_UV_TRIANGLES, ZERO_AREA_TRIANGLES |

## Intentional depot appearance in V08 QA

Fusion32 places locker TypeIds 3497 through 3500 in cities. They keep those logical IDs and container behavior; the Unreal experimental visual resolver displays the imported 3502 depot-chest mesh for each. The source/import/runtime catalog mapping for every item still points to its own imported asset. This is an explicit presentation alias, not an identity substitution in WorldState.

| Fusion32 TypeId | Catalog asset for that ID | Displayed QA mesh |
|---:|---|---|
| 3497 | `/Game/Experimental/V08/ID_03497/SM_V08_03497.SM_V08_03497` | `/Game/Experimental/V08/ID_03502/SM_V08_03502.SM_V08_03502` |
| 3498 | `/Game/Experimental/V08/ID_03498/SM_V08_03498.SM_V08_03498` | `/Game/Experimental/V08/ID_03502/SM_V08_03502.SM_V08_03502` |
| 3499 | `/Game/Experimental/V08/ID_03499/SM_V08_03499.SM_V08_03499` | `/Game/Experimental/V08/ID_03502/SM_V08_03502.SM_V08_03502` |
| 3500 | `/Game/Experimental/V08/ID_03500/SM_V08_03500.SM_V08_03500` | `/Game/Experimental/V08/ID_03502/SM_V08_03502.SM_V08_03502` |
| 1301 | `/Game/Experimental/V08/ID_01301/SM_V08_01301.SM_V08_01301` | `/Game/Experimental/V08/ID_01294/SM_V08_01294.SM_V08_01294` |

The 1301 wall alias is an experimental appearance trial from the in-world note; TypeId 429 remains a stone tile and is not replaced by a wall mesh.

## QA warnings pending visual evaluation

| Warning | Catalog entries |
|---|---:|
| DEGENERATE_UV_TRIANGLES | 4397 |
| ZERO_AREA_TRIANGLES | 2272 |
| PIVOT_NOT_AT_FLOOR | 1253 |
| VERY_FLAT | 19 |

These counts remain warnings; no visual severity conclusion is assigned.

## Mismatches

None.

The registry derives `TypeId` directly from each runtime `item_id`; live WorldState events require a separate test.
