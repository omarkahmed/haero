# update_version_info.sh
# This script generates haero/haero_version.cpp in the build directory, which
# contains version information important for provenance.
# usage: bash update_version_info.sh source_root_dir dest_dir/haero_version.cpp

# Search source_root_dir/CMakeLists.txt for version information.
major_version=`grep '(HAERO_MAJOR_VERSION' $1/CMakeLists.txt | sed -e 's/set (//' -e 's/)$//' -e 's/HAERO_MAJOR_VERSION //'`
minor_version=`grep '(HAERO_MINOR_VERSION' $1/CMakeLists.txt | sed -e 's/set (//' -e 's/)$//' -e 's/HAERO_MINOR_VERSION //'`
patch_version=`grep '(HAERO_PATCH_VERSION' $1/CMakeLists.txt | sed -e 's/set (//' -e 's/)$//' -e 's/HAERO_PATCH_VERSION //'`

# Do we have git?
which_git_wc=`which git | wc -l`

if [ $which_git_wc -gt 0 ]; then
  # Ask git for the revision and for diff information.
  git_revision=`git log -1 --format=format:%h`
  git_diffs=`git diff | wc -l`
else
  git_revision="unknown"
  git_diffs=0
fi

if [ -f $2 ]; then
  cpp_file=$2.tmp
else
  cpp_file=$2
fi

# Write a temporary file with the new contents.
cat > $cpp_file <<- END
// This file is automagically generated by update_version_info.sh.
// DO NOT EDIT!

namespace {
END

echo "// Version string" >> $cpp_file
echo 'const char* _version = "$major_version.$minor_version.$patch_version";' >> $cpp_file
echo "// Git revision (hash) string" >> $cpp_file
echo 'const char* _revision = "$git_revision";' >> $cpp_file
echo "// True iff there are uncommitted changes in your workspace, false if not" >> $cpp_file
if [ $git_diffs -gt 0 ]; then
  echo "bool _has_uncommitted_changes = true;" >> $cpp_file
else
  echo "bool _has_uncommitted_changes = false;" >> $cpp_file
fi
echo "} // anonymous namespace" >> $cpp_file

cat >> $cpp_file <<- END

namespace haero {

const char* version()
{
  return _version;
}

const char* revision()
{
  return _revision;
}

bool has_uncommitted_changes()
{
  return _has_uncommitted_changes;
}

} // namespace haero

END

# If we didn't generate the file in place, compare diffs and move if needed.
if [ "$cpp_file" != "$2" ] && [ -f $2 ]; then
  # Diff the temporary file against what's there.
  tmp_diff_wc=`diff $2 $cpp_file | wc -l`

  # If the files differ, move the temporary file into place. Otherwise
  # discard it.
  if [ $tmp_diff_wc -gt 0 ]; then
    mv $cpp_file $2
  else
    rm -f $cpp_file
  fi
fi


