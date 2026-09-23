# Cuts the REAL33D2D sprite sheets into one PNG per icon, for Resources/UI/Sliced.
#
# The 2D client picks a rectangle out of a shared sheet with `image-clip`. Slate
# cannot do that for a file-backed brush: it packs those into an atlas, so a UV
# region set against the source image samples the atlas instead and the icon
# renders as static from whatever textures happen to sit beside it. Cutting the
# cells on disk sidesteps the atlas entirely -- each brush is then a whole image
# at its native size, which is the shape of every brush here that always worked.
#
# The rectangles below are the `image-clip` values from the .otui that owns each
# control, so re-cutting after an art update is this one command.
#
#   powershell -ExecutionPolicy Bypass -File scripts\client\slice_ui_cells.ps1
Add-Type -AssemblyName System.Drawing
$R = 'C:\Users\dell\Desktop\fusion32\unreal\REAL33D\Resources\UI'
$dst = Join-Path $R 'Sliced'
New-Item -ItemType Directory -Force -Path $dst | Out-Null

function Cell($name, $src, $x, $y, $w, $h) {
  [PSCustomObject]@{ Name=$name; Src=$src; X=$x; Y=$y; W=$w; H=$h }
}

$cells = @(
  Cell 'chrome_minimize'      'Chrome\miniwindow_buttons.png'          0   0 14 14
  Cell 'chrome_restore'       'Chrome\miniwindow_buttons.png'         14   0 14 14
  Cell 'chrome_close'         'Chrome\miniwindow_buttons.png'         28   0 14 14
  Cell 'chrome_close_pressed' 'Chrome\miniwindow_buttons.png'         28  28 14 14
  Cell 'chrome_container_up'  'Chrome\miniwindow_buttons.png'         42   0 14 14
  Cell 'chrome_filter'        'Chrome\miniwindow_buttons.png'         56   0 14 14
  Cell 'chrome_lock_shut'     'Chrome\miniwindow_buttons.png'         84   0 14 14
  Cell 'chrome_lock_open'     'Chrome\miniwindow_buttons.png'         98   0 14 14
  Cell 'chrome_context_menu'  'Chrome\miniwindow_buttons.png'        112   0 14 14
  Cell 'tab_selected'         'Chrome\console_button.png'              0   0 96 18
  Cell 'tab_idle'             'Chrome\console_button.png'              0  18 96 18
  Cell 'panel_right_more'     'Chrome\topstats_button_panel.png'       0   0  9 27
  Cell 'panel_right_less'     'Chrome\topstats_button_panel.png'       9  27  9 27
  Cell 'panel_left_more'      'Chrome\topstats_button_panel.png'      27   0  9 27
  Cell 'panel_left_less'      'Chrome\topstats_button_panel.png'      36  26  9 27
  Cell 'inv_stand'            'Inventory\buttons_general.png'          0   0 20 20
  Cell 'inv_stand_on'         'Inventory\buttons_general.png'         20   0 20 20
  Cell 'inv_follow'           'Inventory\buttons_general.png'          0  20 20 20
  Cell 'inv_follow_on'        'Inventory\buttons_general.png'         20  20 20 20
  Cell 'inv_attack'           'Inventory\buttons_general.png'          0  40 20 20
  Cell 'inv_attack_on'        'Inventory\buttons_general.png'         20  40 20 20
  Cell 'inv_defend'           'Inventory\buttons_general.png'          0  60 20 20
  Cell 'inv_defend_on'        'Inventory\buttons_general.png'         20  60 20 20
  Cell 'inv_balanced'         'Inventory\buttons_general.png'          0  80 20 20
  Cell 'inv_balanced_on'      'Inventory\buttons_general.png'         20  80 20 20
  Cell 'inv_shrink'           'Inventory\min_button_small.png'         0   0 12 12
  Cell 'inv_grow'             'Inventory\max_button_small.png'         0   0 12 12
  Cell 'inv_purse'            'Inventory\purse.png'                    0   0 34 12
  Cell 'arrow_previous'       'ActionBar\arrow-left.png'               0  34 17 17
  Cell 'arrow_next'           'ActionBar\arrow-right.png'              0  34 17 17
  Cell 'arrow_first'          'ActionBar\double-arrow-left.png'        0  34 17 17
  Cell 'arrow_last'           'ActionBar\double-arrow-right.png'       0  34 17 17
  Cell 'battle_filter_idle'   'Options\button_empty.png'               0   0 20 20
  Cell 'battle_filter_on'     'Options\button_empty.png'              20   0 20 20
  Cell 'automap_rose'         'Automap\automap_rose.png'               0   0 43 43
  Cell 'automap_layers'       'Automap\automap_indicator_maplayers.png' 98 0 14 67
  Cell 'automap_fullmap'      'Automap\automap_buttons.png'            0   0 20 20
  Cell 'automap_zoomout'      'Automap\automap_buttons.png'            0  20 20 20
  Cell 'automap_zoomin'       'Automap\automap_buttons.png'            0  40 20 20
)

foreach ($k in 'skills','battlelist','vip','control','options','logout') {
  $cells += Cell "control_$k"     "Options\button_$k.png"  0 0 20 20
  $cells += Cell "control_${k}_on" "Options\button_$k.png" 20 0 20 20
}

$names = 'poisoned','burning','electrified','drunk','manashield','slowed','hasted','logoutblocked'
for ($i = 0; $i -lt 8; $i++) {
  $cells += Cell "condition_$($names[$i])" 'States\player-state-flags.png' ($i*9) 0 9 9
}

$n = 0
foreach ($c in $cells) {
  $img = [System.Drawing.Image]::FromFile((Join-Path $R $c.Src))
  $out = New-Object System.Drawing.Bitmap $c.W, $c.H
  $g = [System.Drawing.Graphics]::FromImage($out)
  $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
  $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::Half
  $srcRect = New-Object System.Drawing.Rectangle $c.X, $c.Y, $c.W, $c.H
  $dstRect = New-Object System.Drawing.Rectangle 0, 0, $c.W, $c.H
  $g.DrawImage($img, $dstRect, $srcRect, [System.Drawing.GraphicsUnit]::Pixel)
  $g.Dispose()
  $out.Save((Join-Path $dst ($c.Name + '.png')), [System.Drawing.Imaging.ImageFormat]::Png)
  $out.Dispose(); $img.Dispose()
  $n++
}
"sliced $n cells into $dst"

