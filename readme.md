## Deps:
Ensure you have both `sdbus` and `scons` installed. 
A better solution will come down the line maybe something like openBLE. 
For now we build for linux and bind to sdbus. Scons is for godot extension build system. 

## Build command:

```
scons platform=linux target=TARGET arch=ARCH

targets=[editor,template_debug,template_release]
arch=[x86_64,arm64]
```


Pick and choose what you need and when.

## In event of issues:

Most likely pain point is the latest godot-cpp tools. You need to specficically build against
version 4.5 since we are till on there. 

###  Check current godot-cpp version

```
cd godot-cpp && git status && git log -1 --oneline
```

### Update godot-cpp to match Godot 4.5

```
git fetch --all
git checkout 4.5.0  # or whatever tag matches your Godot version
```

Then rebuild everything

## Still having problems?

### Expected Build Output
After building, you should see in `bin/`:
- libble_gdextension.linux.editor.ARCH.so
- libble_gdextension.linux.template_debug.ARCH.so  
- libble_gdextension.linux.template_release.ARCH.so

### Known Issues

**Duplicate .gdextension:**

If Godot fails to load, check that there's NO .gdextension file in the `bin/` folder
only the one at the addon root should exist. If it is inside bin, move it to the root
of the addon. 

