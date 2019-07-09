# Build
## Flatpak way

1. install flatpak and flatpak-builder
2. add flathub to the (user) remotes
3. Build with ```flatpak-builder --user --install --install-deps-from=flathub <BUILDDIR> sirEdit/de.marko10-000.sirEdit.json```
4. Run the programm with ```flatpak run de.marko10_000.sirEdit```

## Normal build way

1. create build dir and enter it
2. run ```meson <SOURCE>```
3. run ```ninja```
4. execute ```sirEdit```

# Dependencies

- PEGTL https://github.com/taocpp/PEGTL
- gtkmm https://www.gtkmm.org/en/
- gtest (test only) https://github.com/google/googletest