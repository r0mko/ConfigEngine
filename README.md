# ConfigEngine

A QML way to work with JSON configurations.

## Introduction

This QML engine plugin provides a `ConfigEngine` singleton which enables you to have a multilayered config similar to one in VS Code. For straighforward usage in QML, the hierarchy of the JSON file is represented as QObject properties with notify signals, so they can (and meant to) be used in QML bindings.

Assuming that the root config represents the full set of config values, each next layer, once loaded and activated, just overrides the existing values, but never changes the set of properties. Any layer can be loaded, activated, deactivated and unloaded in runtime, and this will just emit the change signals of affected properties, thus triggering re-evaluation of corresponding QML bindings. 

Configs are stored in JSON format. Thus, for a config like:
```json
{
    "colors": {
        "defaultBackground": "#ffffff",
        "defaultText": "#000000",
    }
}
```
it is possible to access the values in QML through `ConfigEngine.config.colors.defaultBackground` and `ConfigEngine.config.colors.defaultText`. 

## Usage

First, load the root config by calling the `ConfigEngine.loadLayer("filename.json")` for the first time or after calling the `clear()` method. Then, load any number of config layers by calling the same method again with different filenames. Each loaded layer is given a name corresponding the filename without path and extension. 

By default each next layer is loaded on top of the previous, so the last loaded layer has the highest priority, should it be activated along with others. It is possible, however, to change the default order by providing an optional second argument `desiredIndex` argument for `ConfigEngine.loadLayer(path, desiredIndex)`. Please note that if the layer with desired index is already loaded, this will overwrite the existing data without warning. 

Note that loaded layer is not activated by default, in other words, all layers by default are loaded invisible. Use `ConfigEngine.activateLayer(name)` and `ConfigEngine.deactivateLayer(name)` methods to make layers "visible" throught the `ConfigEngine.config` property.

If you need to change the set of activated layers at once without triggering huge number of signal changes for each step, call `ConfigEngine.beginUpdate()`, then activate/deactivate desired layers and then call `ConfigEngine.endUpdate()` method, which triggers all accumulated "changed" signals.

## Example

You can look at the example in [`example`](example) folder. You can also run it with
```bash
qbs run
```
from the root folder of the repository.

## Known issues

* QMake build file [`ConfigEngine.pro`](ConfigEngine.pro) is outdated. Use Qbs instead.
