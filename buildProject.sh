#!/usr/bin/env sh
# Build the portable catalog console and run the local C checks.
set -eu
cd "$(dirname "$0")"
mkdir -p build/native
cc --version
cardSources="CreditCardThing/catalog.c CreditCardThing/catalogBuild.c CreditCardThing/catalogText.c CreditCardThing/catalogWallet.c CreditCardThing/catalogFiles.c CreditCardThing/catalogApp.c CreditCardThing/rewardMath.c CreditCardThing/input.c"
# SQLite is a separate dependency, with separate compiler options.
set -x
cc -std=c11 -O1 -DSQLITE_THREADSAFE=0 -DSQLITE_OMIT_LOAD_EXTENSION -DSQLITE_ENABLE_FTS5 -c thirdParty/sqlite3.c -o build/native/sqlite3.o
cc -std=c11 -Wall -Wextra -Wpedantic -Werror tools/catalogConsole.c $cardSources build/native/sqlite3.o -lm -o build/native/cardClarity
build/native/cardClarity --build-catalog data data/cardCatalog.db
cc -std=c11 -Wall -Wextra -Wpedantic -Werror tests/testCatalog.c $cardSources build/native/sqlite3.o -lm -o build/native/testCatalog
cc -std=c11 -Wall -Wextra -Wpedantic -Werror tests/testCatalogMenu.c tests/testSupport.c -o build/native/testCatalogMenu
cc -std=c11 -Wall -Wextra -Wpedantic -Werror tests/testCatalogScale.c CreditCardThing/catalog.c CreditCardThing/catalogText.c CreditCardThing/catalogFiles.c CreditCardThing/catalogWallet.c CreditCardThing/input.c build/native/sqlite3.o -lm -o build/native/testCatalogScale
testFolder="$(mktemp -d "$PWD/build/catalogChecksXXXXXX")"
build/native/testCatalog data/cardCatalog.db "$testFolder/cases"
build/native/testCatalogMenu "$PWD/build/native/cardClarity" "$PWD/data/cardCatalog.db"
build/native/testCatalogScale data/cardCatalog.db "$testFolder/scale.db"
