# libktorrent

Qt-based implementation of the BitTorrent protocol

## Introduction

libktorrent provides classes to add BitTorrent support to Qt-based applications.

Supported features include:
* Queuing of torrents
* Global and per torrent speed limits
* Importing of partially or fully downloaded files
* File prioritisation for multi-file torrents
* Selective downloading for multi-file torrents
* Kick/ban peers with an additional IP Filter dialogue for list/edit purposes
* UDP tracker support
* Support for private trackers and torrents
* Support for µTorrent's peer exchange
* Support for protocol encryption (compatible with Azureus)
* Support for creating trackerless torrents
* Support for distributed hash tables (DHT, the Mainline version)
* Support for UPnP to automatically forward ports on a LAN with dynamic assigned hosts
* Support for webseeds
* Tracker authentication support
* Connection through a proxy (currently broken)
