# BetterSave

Improves local level saving by saving each created level as an individual `.gmd` file rather than dumping everything in `CCLocalLevels`. Should result in a significant decrease in level save times, as well as lessen the likelihood of the game crashing while saving.

Levels are saved in `<GD save dir>/levels`.

Also includes a trashcan system, where levels are placed in a trashcan before being permanently deleted.

## Developers Looking To Extend The System

If you're looking to leveradge BetterSave's systems and store other level categories inside `<GD save dir>/levels`, please consider the following.

BetterSave assigns all levels unique IDs by looking at the names of the directories in each subdirectory in `<GD save dir>/levels`. In other words, if a folder exists at `<GD save dir>/levels/epic/my-awesome-level`, then `my-awesome-level` will be considered a reserved ID. This is to avoid ID conflicts when trashing / untrashing levels; you should always follow this pattern to ensure compatability with BetterSave features.

BetterSave internally uses a category system to determine whether a level is trashed or a created level. In the future, this category system may be exposed as an API, if there is an use case to do so. Currently it's internal-use only as I don't think there's a point to new categories other than a trashcan.
