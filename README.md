# deathtrap

C++ library and WebSocket server for accessing Hydrus databases. Use at your own risk.

# Status of Hydrus database feature support

| Feature | Implementation status |
| --- | --- |
| Query database version | DONE |
| Query file storage paths | DONE |
| *Lock/release database (for safe external backups) | DONE |
| SQLite maintenance: vacuum | DONE |
| *SQLite maintenance: vacuum into | DONE |
| SQLite maintenance: analyze | DONE |
| SQLite maintenance: integrity check | DONE |
| *SQLite maintenance: optimize | DONE |
| *SQLite operations: shrink memory | DONE |
| *SQLite maintenance: set number of auxiliary threads | DONE |
| Serve files/thumbnails | DONE |
| Serve files/thumbnails: SSL | DONE |
| Create new database | no |
| Query file metadata (incl. alt. group id) | no |
| Search: text query parsing | no |
| Search: search in database | no |
| Search: autocomplete | no |
| *Search: include non-local file results from tag repos | no |
| File management: add/import files | no |
| Manage tags | no |
| Clean/normalize tags | no |
| Manage URLs | no |
| Manage notes | no |
| Manage other metadata | no |
| Services: add/delete | no |
| Delete/undelete files | no |
| Duplicate system: alternate groups | no |
| Duplicate system: search & processing | no |
| Duplicate system: other operations | no |
| Query saved sessions | no |
| Query subscription data | no |
| Manage file viewing statistics | no |
| Recent & related tags | no |
| Tag migration | no |
| Query tag parents | no |
| Query tag siblings | no |
| Query options | no |
| Manage tag parents | no |
| Manage tag siblings | no |
| Manage pending status | no |
| Downloading updates from tag repositories | no |
| Uploading pending data to tag repositories | no |
| Processing tag repository updates | no |
| Resetting, reprocessing tag repositories | no |
| Relocate client files/rebalance | no |
| File maintenance operations | no |
| Clear orphans | no |
| Repair invalid tags | no |

Features marked with `*` are not present in Hydrus.

Not currently planned:
* Anything to do with the Hydrus server
* Anything to do with the download system
* Backup/restore
* Remote file repositories, updates
* Query/manage db password
