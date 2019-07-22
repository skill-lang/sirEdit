# Build
## Flatpak way

1. install flatpak and flatpak-builder
2. add flathub to the (user) remotes `flatpak remote-add --user --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo`
3. Build with `flatpak-builder --user --force-clean --install --install-deps-from=flathub <BUILDDIR> sirEdit/de.marko10-000.sirEdit.json`
   Be careful, `<BUILDDIR>` and the current path should **NOT** be in the directory of the git.
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