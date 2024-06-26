# <cy>BetterSave</c>

Improves saving speed in the editor & avoids data loss by temporarily saving editor levels into a separate file.

## <cg>No longer deletes levels!</c>

The original version of BetterSave had an issue where updating GD would cause levels to disappear, though while being recoverable. **This version no longer causes data loss.**

Do note however that **GD itself and other mods can still cause data loss**. BetterSave does not protect you from other crashes, only from itself.

## <cp>Recovering old levels</c>

BetterSave v2 can recover levels and lists lost by older pre-2.206 versions of BetterSave. It will do this automatically on startup. The recovery will skip duplicate levels and lists, determined by level data, so exact copiables may not be recovered.

Note that for safety, <cp>the recovery will <cy>not</c> delete the save directory</c>. If you are sure everything has been recovered and want to free up space, you can manually delete the `levels` folder from your GD save directory.

## <co>Trashcan</c>

BetterSave introduces a trashcan system where levels are placed before they are deleted. You can <cg>recover levels</c> from the trashcan located at the Created Levels layer. <cr>Trashcan items are <cy>not</c> automatically deleted ever</c>; if you want to permanently delete a level, you need to open up the trashcan and manually do so.
