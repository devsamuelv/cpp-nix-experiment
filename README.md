# Nix Camera Experiment

This was an experiment with nix/c++ and opencv. I wanted to see if nix could be a useful solution for big projects on embedded systems. Since nix has support for some popular embbeded boards I thought it was worth pursuing.

## TODO:

- [ ] Add multithread support for camera streams.
- [X] Working camera stream.

## Libraries

The libraries are added via nix packages housed inside the `libraries` folder. Currently the only library being used is `httplib`. NOTE: When adding new libraries the nix package for each respective library must be added to the packages declaration inside `default.nix` and `shell.nix`. Adding it to `shell.nix` will add it to the develop environment which is kinda useful.
