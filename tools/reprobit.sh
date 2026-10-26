#!/usr/bin/env bash

cd /var/home/johnrowl/.codex/worktrees/c71a/lrr-portable

rbit source preview /var/home/johnrowl/.codex/worktrees/c71a/lrr-portable
rbit source lock /var/home/johnrowl/.codex/worktrees/c71a/lrr-portable

# rbit import cmake /var/home/johnrowl/.codex/worktrees/c71a/lrr-portable \
#   --cmake-define PLATFORM=winapi \
#   --cmake-define LRR_CLASSIC_MSVC5=ON

GIT_CEILING_DIRECTORIES=/var/home/johnrowl/.codex/worktrees/c71a/lrr-portable/.reprobit-state/runs \
REPROBIT_MSVC_5_0_RTM_ROOT=/var/home/johnrowl/.local/share/reprobit/toolchains/msvc_5_0_rtm \
rbit import cmake /var/home/johnrowl/.codex/worktrees/c71a/lrr-portable \
  --refresh --cmake cmake --configuration RelWithDebInfo --timeout 600 \
  --cmake-define PLATFORM=winapi \
  --cmake-define LRR_CLASSIC_MSVC5=ON \
  --directive-input program=msvcrt.lib \
  --directive-input program=oldnames.lib

REPROBIT_MSVC_5_0_RTM_ROOT=/var/home/johnrowl/.local/share/reprobit/toolchains/msvc_5_0_rtm \
  rbit build /var/home/johnrowl/.codex/worktrees/c71a/lrr-portable

REPROBIT_MSVC_5_0_RTM_ROOT=/var/home/johnrowl/.local/share/reprobit/toolchains/msvc_5_0_rtm \
  rbit repair /var/home/johnrowl/.codex/worktrees/c71a/lrr-portable
#
