#!/usr/bin/env bash
./scripts/make.sh $1 os-x compile-clean-without-finish
./scripts/make.sh $1 linux compile-clean-without-finish
./scripts/make.sh $1 windows compile-clean-without-finish
