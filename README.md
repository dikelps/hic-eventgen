# hic-eventgen

_heavy-ion collision event generator_

## Three-sentence summary

- This is a workflow for running simulations of relativistic heavy-ion collisions.
- The primary intent is generating large quantities of events for use in Bayesian parameter estimation projects.
- It includes scripts and utilities for running on the [Open Science Grid (OSG)](https://www.opensciencegrid.org) and [NERSC](https://www.nersc.gov), and could run on other systems.

## Physics models

The collision model consists of the following stages:

- [trento](https://github.com/Duke-QCD/trento) — initial conditions
- [freestream](https://github.com/Duke-QCD/freestream) — pre-equilibrium
- [OSU hydro](https://github.com/jbernhard/osu-hydro) — viscous 2+1D hydrodynamics
- [frzout](https://github.com/jbernhard/frzout) — particlization
- [UrQMD](https://github.com/jbernhard/urqmd-afterburner) — hadronic afterburner

Each is included as a git submodule in the [models](models) directory.

:warning: git submodules have some annoying behavior.
__Use the `--recursive` option when cloning this repository to also clone all submodules.__
I suggest skimming the [section on submodules in the Pro Git book](https://git-scm.com/book/en/v2/Git-Tools-Submodules).

## Installing locally

hic-eventgen is probably most useful on [computational systems such as OSG and NERSC](#computational-systems), but it can be installed locally for testing or running a few events.

Prerequisites:

- Python __3.5+__ with numpy, scipy, and h5py
- C, C++, and Fortran compilers
- CMake 3.4+
- Boost and HDF5 C++ libraries

Clone the repository with the `--recursive` option to acquire all submodules.

The models must be installed into an active Python [virtual environment](https://docs.python.org/3/library/venv.html) (venv) or a [conda environment](https://conda.io/docs/user-guide/tasks/manage-environments.html) (I suggest using [Miniconda](https://conda.io/miniconda.html)).

If Python and its packages are already installed on your system, the easiest choice is probably a venv:

    python -m venv --system-site-packages --without-pip /path/to/venv
    source /path/to/venv/bin/activate

Or if using conda:

    conda create -n hic numpy scipy h5py
    source activate hic

After creating and activating the environment, simply run the [install](install) script.

Before running events:

- The environment must be active.
  This can of course be accomplished by sourcing the `activate` script as usual, but all that's actually necessary is for `<environment_prefix>/bin` to be in your `PATH`.
  To verify this, check if your python executable points to `<environment_prefix>/bin/python` (run `which python`).
- The environment variable `XDG_DATA_HOME` must be set to `<environment_prefix>/share` so that `osu-hydro` can find its data files.

In the venv example above, these criteria can be satisfied by running

    export PATH="/path/to/venv/bin:$PATH"
    export XDG_DATA_HOME="/path/to/venv/share"

Alternatively, you can use [run-events-wrapper](models/run-events-wrapper), a simple script that sets the necessary variables and then forwards all arguments to `run-events` (see next section).
It is installed to the `bin` directory of the environment, alongside `run-events` itself.
You can run it using its full path:

    /path/to/venv/bin/run-events-wrapper arguments...

## Running events

The Python script [models/run-events](models/run-events) executes complete events and computes observables.
The basic usage is

    run-events [options] results_file

Observables are written to `results_file` in __binary__ format (see [event data format](#event-data-format) below).

The most common options are:

- `--nevents` number of events to run (by default, events run continuously until interrupted)
- `--nucleon-width` Gaussian nucleon width (passed to `trento` and used to set the hydro grid resolution)
- `--trento-args` arguments passed to `trento`
  - __must__ include the collision system and cross section
  - __must not__ include the nucleon width or any grid options
- `--tau-fs` free-streaming time
- `--hydro-args` arguments  passed to `osu-hydro`
  - __must not__ include the initial time, freeze-out energy density, or any grid options
- `--Tswitch` particlization temperature

:warning: Options `--trento-args` and `--hydro-args` are passed directly to the respective programs.
Ensure that the restrictions described above are satisfied.
See also the docs for [trento](http://qcd.phy.duke.edu/trento) and [OSU hydro](https://github.com/jbernhard/osu-hydro).

See `run-events --help` for the complete list of options.

### The hydro grid

The computational grid for `osu-hydro` is determined adaptively for each event in order to achieve sufficient precision without wasting CPU time.

The grid cell size is set proportionally to the nucleon width (specifically 15% of the width).
So when the nucleon width is small, events run a fine grid to resolve the small-scale structures;
for large nucleons, events run on a coarser (i.e. faster) grid.

The physical extent of the grid is determined by running each event on a coarse grid with ideal hydro and recording the maximum size of the system.
Then, the event is re-run on a grid trimmed to the max size.
This way, central events run on large grids to accommodate their transverse expansion, while peripheral events run on small grids to save CPU time.
Although pre-running each event consumes some time, this strategy is still a net benefit because of all the time saved from running peripheral events on small grids.

### Event data format

Event observables are written in __binary__ format with the data type defined in `run-events` (do a text search for "results = np.empty" to find it in the file).
Many results files may be concatenated together:

    cat /path/to/results/* > events.dat

In Python, read the binary files using [numpy.fromfile](https://docs.scipy.org/doc/numpy/reference/generated/numpy.fromfile.html).
This returns [structured arrays](https://docs.scipy.org/doc/numpy/user/basics.rec.html) from which observables are accessed by their field names:

```python
import numpy as np
events = np.fromfile('events.dat', dtype=<full dtype specification>)
nch = events['dNch_deta']
mean_pT_pion = events['mean_pT']['pion']
```

It's probably easiest to copy the relevant `dtype` code from `run-events`.

I chose this plain binary data format because it's fast and space-efficient.
On the other hand, it's inconvenient to fully specify the dtype when reading files, and organizing many small files can become unwieldy.

A format with metadata, such as HDF5, is not a good choice because each event produces such a small amount of actual data.
The metadata takes up too much space relative to the actual data and reading many small files is too slow.

The best solution would probably be some kind of scalable database (perhaps MongoDB), but I simply haven't had time to get that up and running.
If someone wants to do it, by all means go ahead!

### Input files

Options for `run-events` may be specified on the command line or in files.
Files must have one option per line with `key = value` syntax, where the keys are the option names without the `--` prefix.
After creating an input file, use it with the syntax `run-events @path/to/input_file`.

For example, if a file named `config` contains the following:

    nevents = 10
    nucleon-width = 0.6

Then `run-events @config` is equivalent to `run-events --nevents 10 --nucleon-width 0.6`.

Input files are useful for saving logical groups of parameters, such as for a set of design points.

## Computational systems

- [Open Science Grid (OSG)](osg)
- [NERSC](nersc)
