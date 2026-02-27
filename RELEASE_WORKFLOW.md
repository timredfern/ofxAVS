the release workflow is:

  avs_lib:
  1. Update main from master (minus dev files)
  2. Tag & push to GitHub

  ofxAVS:
  1. Update main from master (minus dev files)
  2. cd libs/avs_lib && git checkout main && git pull github main
  3. Commit (captures new submodule ref)
  4. Tag & push to GitHub

  The submodule commit hash gets recorded when you git add libs/avs_lib.


-----

For GitHub releases you have a few options:

  1. GitHub CLI (manual)
  gh release create v1.0.0 --title "v1.0.0" --notes "Initial release"
  gh release upload v1.0.0 ./path/to/AVS_standard.dmg

  2. GitHub Actions (automated)
  Create .github/workflows/release.yml that builds on tag push and uploads artifacts.

  3. Web interface
  Go to Releases → Draft new release → Upload binaries manually.
