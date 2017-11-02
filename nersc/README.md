# NERSC

See https://www.nersc.gov/users/getting-started for general information.

## Installation

The models must be installed in a conda environment in the NERSC [global common file system](https://www.nersc.gov/users/storage-and-file-systems/file-systems/global-common-file-system), which is intended for software installation.

Download and install [Miniconda](https://conda.io/miniconda.html) on a NERSC machine:
```bash
wget https://repo.continuum.io/miniconda/Miniconda3-latest-Linux-x86_64.sh
bash Miniconda3-latest-Linux-x86_64.sh
```
The default installation location `~/miniconda3` is fine.

Tip: to avoid having to source the conda activate script every time you login to NERSC, prepend the conda `bin` directory to your `PATH`.
Add the following to your `~/.bashrc.ext`:
```bash
export CONDA_PREFIX=$HOME/miniconda3
export PATH="$CONDA_PREFIX/bin:$PATH"
```

Create and activate an environment with the required libraries:
```bash
conda create -p /global/common/software/<project>/<prefix> numpy scipy cython h5py
source activate /global/common/software/<project>/<prefix>
```
where `<project>` is your NERSC project name and `<prefix>` is your desired name for the environment.

Now run the [install](install) script.

## Running jobs

I recommend running events on Edison or Cori Haswell, but not Cori KNL (these codes make poor use of the KNL architecture and so it would be an inefficient way to spend CPU hours).
I have set the compiler flags in the `install` script to target Edison and Cori Haswell.

Before running heavy-ion collision events, peruse the [NERSC documentation on running jobs](http://www.nersc.gov/users/computational-systems/cori/running-jobs), look at their example batch scripts for [Cori](https://www.nersc.gov/users/computational-systems/cori/running-jobs/example-batch-scripts) and [Edison](https://www.nersc.gov/users/computational-systems/edison/running-jobs/example-batch-scripts), and run some generic test jobs.

The [examples](examples) folder contains some sample batch scripts, discussed below.
__Please read the scripts carefully and modify them for your needs.
They will NOT work without some changes!__

### A single parameter point

[examples/simple](examples/simple) shows how to run some events all at the same parameter point.
It sets the necessary environment variables (be sure to replace `<project>` and `<prefix>` in `CONDA_PREFIX`) and then executes `run-events` via `srun`.

__Number of tasks:__
I don't like the way the NERSC examples explicitly give the `-n` option to `srun`, because it depends on both the number of CPUs per node (which is different on Cori and Edison) and the number of nodes (which could be different for every job).
Instead, I set the `--cpus-per-task` sbatch option so that every CPU runs a single task, regardless of the number of CPUs per node or the number of nodes.

__Filesystem:__
I use Cori scratch for all output files.
The `$CSCRATCH` environment variable points to your user directory on Cori scratch.

__Process rank:__
`run-events` is treated as an MPI executable via `srun` (although the processes don't communicate with each other).
Option `--rankvar SLURM_PROCID` is necessary so that `run-events` can determine its process rank (note that this is an option to `run-events`, not to `srun`).
`run-events` reads its process rank from the environment variable and appends the rank to its output files, e.g. `/path/to/results.dat` becomes `/path/to/results/rank.dat`.
The `--rankfmt` option allows formatting of the rank in filenames.

__Email notifications:__
If you want email notifications for job status updates, enter your address for the `--mail-user` sbatch option.

### Multiple parameter points

[examples/design](examples/design) demonstrates a strategy for running a set of parameter design points from input files.
It's similar to the "simple" example, but uses the intermediate script [examples/design-wrapper](examples/design-wrapper) to assign each CPU to a design input file in round-robin fashion.

### Checkpoints (important!)

The "simple" and "design" examples both run events continuously until the job times out;
this maximizes CPU time efficiency since there are never any idle CPUs.
If a job were to instead run a fixed number of events on each CPU, some would finish earlier than others and those CPUs would be idle until the last one finishes.
That usage is still charged, even if the CPUs are idle!

But there's a problem with running events until timeout.
The last event on each CPU is always interrupted, so longer-running events are more likely to be cut off.
And since more central events take longer, this introduces a centrality bias into the event sample.

__More generally: once an event begins, it is part of the cross section, and it must be completed!__

The solution to this is job checkpoints.
When the `--checkpoint` option is given to `run-events`, before it starts each event it writes a checkpoint file containing the initial condition and all options in Python pickle format.
Example:
```bash
run-events --checkpoint checkpoint.pkl --logfile events.log results.dat
```
If and when the event completes, the checkpoint file is deleted;
if it does not complete, it can be resumed later from the checkpoint file using the syntax `run-events checkpoint <checkpoint_file>`.
Continuing the example:
```bash
run-events checkpoint checkpoint.pkl
```
This will append to the original log and results files and delete the checkpoint file upon completion.

[examples/checkpoints-gnu-parallel](examples/checkpoints-gnu-parallel) shows how to run a batch of checkpoints using [GNU Parallel](https://www.gnu.org/software/parallel), which is suitable for a __single node__.

[examples/checkpoints-taskfarmer](examples/checkpoints-taskfarmer) shows how to use [taskfarmer](https://www.nersc.gov/users/data-analytics/workflow-tools/taskfarmer), which can run on many nodes and is thus useful if you have too many checkpoints to run on one node in a reasonable time.
