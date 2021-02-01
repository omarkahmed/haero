# This file loads all necessary modules for building MAM on PNNL's Compy Machine

source /etc/profile.d/modules.sh

# Load relevant modules
module purge && module load cmake/3.11.4 gcc/8.1.0  mvapich2/2.3.1 netcdf/4.6.3


# Set third-party library paths
export HDF5_DIR=/share/apps/hdf5/1.10.5/serial
export HDF5_INCLUDE_DIR=$HDF5_DIR/include
export HDF5_LIBRARY_DIR=$HDF5_DIR/lib
export HDF5_LIBRARY=libhdf5.a
export HDF5_HL_LIBRARY=libhdf5_hl.a

export NETCDF_INCLUDE_DIR=$NETCDF_INCLUDE
export NETCDF_LIBRARY_DIR=$NETCDF_LIB
export NETCDF_LIBRARY=libnetcdf.a

export NETCDFF_INCLUDE_DIR=$NETCDF_INCLUDE_DIR
export NETCDFF_LIBRARY_DIR=$NETCDF_LIBRARY_DIR
export NETCDFF_LIBRARY=libnetcdff.a

# Set relevant compilers.
export FC=gfortran
export CXX=mpicxx

# Set extra LDFLAGS
export EXTRA_LDFLAGS=-lcurl
