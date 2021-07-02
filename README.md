# ConfigEngine

A QML way to work with JSON configurations.

## Introduction

This QML engine plugin provides a `ConfigEngine` singleton which enables you to have a multilayered config similar to one in VS Code.
There is a `ConfigEngine::Global` level which is considered a read-only config (usually bundled with the app). In turn, `ConfigEngine::User` corresponds to a user's config usually resided in `~/.config/<your_app>` directory. The last level is `ConfigEngine::Project` and it represents the config for a workspace.

Assuming that the global config always contains a full set of possible config values, each next level, when being loaded, just amends the global config overriding existing values. User and project configs can be loaded and unloaded in runtime without any problem via corresponding methods. Whenever a value of a specific config key is changed due to the amendment process or somehow externally, a signal is emitted and the corresponding property is notified triggering invalidation (and re-evaluation) of a binding.

To access actual values, `ConfigEngine` also exposes a global context property `Config`.

Configs are stored in JSON format. Thus, for a config like:
```json
{
	"colors": {
		"defaultBackground": "#ffffff",
		"defaultText": "#000000",
	}
}
```
it is possible to access the values in QML through `Config.colors.defaultBackground` and `Config.colors.defaultText`.

## Usage

To add the plugin to your Qbs project, simply include `src/plugin.qbs`.

## Example

You can look at the example in [`example`](example) folder. You can also run it with
```bash
qbs run
```
from the root folder of the repository.

## Known issues

* QMake build file [`ConfigEngine.pro`](ConfigEngine.pro) is outdated. Use Qbs instead.
