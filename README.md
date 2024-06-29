# BetterSave

No longer does anything related to saving except for the trashcan system. Will have backups in the future.

## No longer deletes levels!

The original version of BetterSave had an issue where updating GD would cause levels to disappear, though while being recoverable. **This version no longer causes data loss.**

Do note however that **GD itself and other mods can still cause data loss**. BetterSave does not protect you from other crashes, only from itself.

## Recovering old levels

BetterSave v2 can recover levels and lists lost by older pre-2.206 versions of BetterSave. It will do this automatically on startup. The recovery will skip duplicate levels and lists, determined by level data, so exact copiables may not be recovered.

Note that for safety, the recovery will not delete the save directory. If you are sure everything has been recovered and want to free up space, you can manually delete the `levels` folder from your GD save directory.

## Trashcan

BetterSave introduces a trashcan system where levels are placed before they are deleted. You can recover levels from the trashcan located at the Created Levels layer. Trashcan items are not automatically deleted ever; if you want to permanently delete a level, you need to open up the trashcan and manually do so.
