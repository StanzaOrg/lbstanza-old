#!/bin/bash
set -Eeuxo pipefail
PS4='>>> '

# Given a SemVer version string and a branch name, modify the version number to match Stanza version numbering practices

# Given MAJ.MIN.PAT-TAG.SUF  such as  1.2.34-rc.56

# Number of digits for PAT and SUF should be no more than 2 digits
# For branch "develop"
#   the TAG part of the semver should be "rc"
# For any other branch
#   the TAG part of the semver should be the branch name
#   and the MAJ digit should be zero


SEMVER=${1:?Usage: $0 1.2.34-foo.56 branchname}
BRANCH=${2:?Usage: $0 1.2.34-foo.56 branchname}

## Verify expected input format
grep -E -q '^([0-9]*)\.([0-9]*)\.([0-9][0-9]?)(-((-|_|[a-z]|[A-Z]|[0-9])*)\.([0-9][0-9]?))?$' <<< $SEMVER || \
    { printf "\n\n*** ERROR: input $SEMVER not in format 1.2.34-foo.56\n\n\n" && exit -2 ; }

# MAJ.MIN.PAT-TAG.SUF
# 0.12.0-foo.0 => 0.12.90000
# 0.12.0-foo.3 => 0.12.90003
# 0.12.3-foo.0 => 0.12.90300
# 0.12.34-foo.56 => 0.12.93456
# 0.12.345-foo.678 => fail
MAJ=$(echo $SEMVER | cut -d. -f1)
MIN=$(echo $SEMVER | cut -d. -f2)
PAT=$(echo $SEMVER | cut -d. -f3 | cut -d- -f1)
TAG=$(echo $SEMVER | cut -d. -f3 | cut -d- -f2-)
SUF=$(echo $SEMVER | cut -d. -f4)

[[ $BRANCH == "rc" ]] && \
    printf "\n\n*** ERROR: branch name cannot be \"rc\"\n\n\n" && exit -2

STANAZ_VERSION=$(printf "%d.%d.%d-%s.%d" "$MAJ" "$MIN" "$PAT" "$TAG" "$SUF")

# If the branch is "develop"
if [[ "${BRANCH}" == "develop" ]] ; then
  # and the semver is a final release version 1.2.34
  if [[ "${SEMVER}" == "${MAJ}.${MIN}.${PAT}" ]] ; then
    # output the version unchanged
    STANZA_VERSION="${SEMVER}"
  # else force the tag to be "rc"
  else
    TAG="rc"
    STANZA_VERSION=$(printf "%d.%d.%d-%s.%d" "$MAJ" "$MIN" "$PAT" "$TAG" "$SUF")
  fi

# else the branch is not "develop"
# then make the MAJ part zero
# and the TAG part the branch name
# to avoid colliding with real rc or final versions
else
  if [[ "${SEMVER}" == "${MAJ}.${MIN}.${PAT}" ]] ; then
    MAJ=0
    STANZA_VERSION=$(printf "%d.%d.%d" "$MAJ" "$MIN" "$PAT")
  else
    MAJ=0
    TAG=${BRANCH}
    STANZA_VERSION=$(printf "%d.%d.%d-%s.%d" "$MAJ" "$MIN" "$PAT" "$TAG" "$SUF")
  fi
fi

echo ${STANZA_VERSION}
