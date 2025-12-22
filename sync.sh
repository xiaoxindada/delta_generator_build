#!/bin/bash
LOCALDIR=$(pwd)

USE_MIRROR=true
TAG="android-16.0.0_r4"
FETCH_FROM="https://android.googlesource.com"
[[ "$USE_MIRROR" == true ]] && FETCH_FROM="https://mirrors.tuna.tsinghua.edu.cn/git/AOSP"

GIT_CLONE_ARGS=(
    "--depth=1"
)

GIT_CLONE_REPOS=(
    "platform/system/update_engine"
    "platform/external/avb"
    "platform/external/gflags"
    "platform/external/jsoncpp"
    "platform/external/boringssl"
    "platform/system/core"
    "platform/external/libchrome"
    "platform/external/xz-embedded"
    "platform/external/lzma"
    "platform/external/libbrillo"
    "platform/external/bsdiff"
    "platform/external/bzip2"
    "platform/external/brotli"
    "platform/external/fec"
    "platform/external/puffin"
    "platform/system/extras"
    "platform/external/zlib"
    "platform/external/zstd"
    "platform/external/lz4"
    "platform/system/logging"
    "platform/system/libbase"
    "platform/external/libevent"
    "platform/system/libziparchive"
    "platform/external/libdivsufsort"
    "platform/external/zucchini"
    "platform/external/erofs-utils"
    "platform/external/selinux"
    "platform/external/e2fsprogs"
    "platform/external/squashfs-tools"
    "platform/external/fmtlib"
    "platform/external/modp_b64"
    "platform/external/googletest"
    "platform/external/pcre"
    "platform/system/tools/mkbootimg"
    "platform/external/abseil-cpp"
    "platform/external/protobuf"
)

GIT_CLONE_VERSION_REPOS=(
    
)

echo "Sync all repos from: ${FETCH_FROM}"
echo "Sync branch: ${TAG}"
echo "Git clone args: ${GIT_CLONE_ARGS[@]}"

# disable detached HEAD warning
git config --global advice.detachedHead false

function clone() {
    local repo=$1
    local clone_dir=$2
    local repo_name="${repo##*/}"
    
    echo "Clone repo: ${repo} to ${clone_dir}/${repo_name} ..."
    if [ -d "${clone_dir}/${repo_name}" ]; then
        echo "Directory ${clone_dir}/${repo_name} already exists, skipping clone."
        return
    fi
    if git clone "${FETCH_FROM}/${repo}" -b "${TAG}" "${GIT_CLONE_ARGS[@]}" "${clone_dir}/${repo_name}"; then
		find "${clone_dir}/${repo_name}" -name ".git" | xargs rm -rf {}
        echo "Clone repo: ${repo} success."
    else
        echo "Clone repo: ${repo} failed."
        exit 1
    fi
}

function clone_version() {
    local repo=$1
    local version=$2
    local clone_dir=$3
    local repo_name="${repo##*/}"
    
    echo "Clone repo: ${repo} to ${clone_dir}/${repo_name} ..."
    if [ -d "${clone_dir}/${repo_name}" ]; then
        echo "Directory ${clone_dir}/${repo_name} already exists, skipping clone."
        return
    fi
    if git clone "${FETCH_FROM}/${repo}" -b "${TAG}" "${GIT_CLONE_ARGS[@]}" "${clone_dir}/${repo_name}"; then
		find "${clone_dir}/${repo_name}" -name ".git" | xargs rm -rf {}
        cd ${clone_dir}/${repo_name}
        git fetch --unshallow
        if git reset --hard ${version}; then
            echo "Clone version repo: ${repo} ver: ${version} success."
        else
            echo "Clone version repo: ${repo} ver: ${version} failed."
            exit 1
        fi
        cd ${LOCALDIR}
    else
        echo "Clone version repo: ${repo} ver: ${version} failed."
        exit 1
    fi
}

for repo in "${GIT_CLONE_REPOS[@]}"; do
    clone "${repo}" "${LOCALDIR}/src"
done
for repo in "${GIT_CLONE_VERSION_REPOS[@]}"; do
    repo_fetch_name=$(echo ${repo} | cut -d " " -f 1)
    repo_version=$(echo ${repo} | cut -d " " -f 2)
    clone_version "${repo_fetch_name}" "${repo_version}" "${LOCALDIR}/src"
done
echo "Sync done!"
