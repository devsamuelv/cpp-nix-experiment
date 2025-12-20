# Nix Camera Experiment

This was an experiment with nix/c++ and opencv. I wanted to see if nix could be a useful solution for big projects on embedded systems. Nix has support for some popular embbeded boards, so I thought it was worth pursuing.

## TODO:

- [X] Add multithread support for camera streams.
- [X] Working camera stream.
- [X] Display image segmentation masks from pytorch models.
- [ ] Optimize the ML segmentation camera stream for realtime speeds.
- [ ] Improve segmentation accuracy. (Current masks are not useable for navigation)
- [ ] Modularize codebase to allow seperation of tasks amongst threads and classes.

## Libraries

The libraries are added via nix packages housed inside the `libraries` folder. The libraries folder current has three libraries residing in it. `Pytorch` and `httplib`, while `spdlog` is included it is not integrated with the main `flake.nix` file. (Due to some compile errors) The reasoning behind using nix for dependencies instead of the countless other package managers for c++ or any other language. Is reliability and cross-compliation, by building the source code for each library we can avoid the massive headache of sourcing binanaries and dependencies for libraries when deploying to arm based systems.

## Building & Running

First, because nix is awesome you don't need to download anything (except for nix). After cloning the repo make sure you have [nix flakes enabled](https://nixos.wiki/wiki/Flakes). Then execute this command `./build-run.sh` it will take some time. But when its finished the camera server will be running on `http://localhost:8080/stream`!

There are three main nix commands that you will need to know to develop with nix. It's the `build` and `develop` commands prefixed with `nix` like so: `nix develop` or `nix run`. Now for the next question what do all three really do?

#### `nix build`

It will compile the project according to the nix configuration files like `flake.nix` or and other `.nix` file. CMake is used for building the raw executable, but nix manages the steps and environment that cmake receives to build the binaries in. When its completed a new folder named `result` will be available with the compiled binary ready.

#### `nix run`

Cannot use `nix run` because the ros libraries are not packaged into the binary. This means you need to build the program and then execute it in the shell provided by `nix develop`. There is a script to make this easier called `build-run.sh`

#### `nix develop`

It will compile the devshell which is a custom development environment with all the libraries and headers necessary to develop this project. Its nice when executed you have a full fledged dev environment without any configuration necessary as a new developer. 

## Debugging

I decided to use valgrind for memory debugging since its reliable and its been around for awhile. Use the command below to execute teleop-control with valgrind. Make sure you run `nix build` before using valgrind since it doesn't compile the binary by itself.

```sh
valgrind --leak-check=yes ./result/bin/teleop-control
```

## Developing on WSL
Write something saying how wsl is not plug and play for this project and that native linux is much easier to configure.