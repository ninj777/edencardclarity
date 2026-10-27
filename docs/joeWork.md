# Joe: sample cards and card IDs

Joe Kwon's proposal work is the card records, example rates, catalog checks, and later benefit research. Gate 1 needs at least three clearly labeled samples. Gate 2 needs the saved IDs to keep matching those samples.

The assigned source is [cards.h](../CreditCardThing/cards.h) and [cards.c](../CreditCardThing/cards.c). Together they contain 257 lines. Most of the catalog was already present in the uploaded version. The Gate 1 changes were 55 added and 19 removed lines. The later changes were 17 added and 9 removed lines. [codeChanges.md](codeChanges.md) explains the counts.

## The five records

All names, fees, and rates below are practice values. They are not verified offers.

- ID 1: Travel Sample. Visa. Points. Base rate 1.25%. Example annual fee $95. Bonuses: airlines +5%, stores +3%, services +2%.
- ID 2: Points Sample. American Express. Points. Base rate 1%. Example annual fee $250. Bonuses: airlines +4%, stores +2%, services +1.5%.
- ID 3: Cashback Sample. Mastercard. Cashback. Base rate 1%. Example annual fee $0. Bonuses: airlines +3%, stores +2%, services +1%.
- ID 4: Everyday Sample. American Express. Cashback. Base rate 1%. Example annual fee $0. Bonuses: airlines +3%, stores +2%, services +2%.
- ID 5: Starter Sample. Discover. Cashback. Base rate 1%. Example annual fee $0. Bonuses: airlines +4.5%, stores +2.5%, services +1%.

`initTestCards` creates these records with `registerCard`, then sets their three example bonuses with `addMultiplier`. The same records are used in the console, the window, and the module tests.

## What a Card contains

The `Card` struct is declared in `cards.h`. It keeps the related fields together:

- `issuer`: a `char*` pointing to the name string. The current names are string literals and must not be changed through this pointer.
- `network`: an enum value for Visa, Mastercard, American Express, or Discover.
- `annualFee`: an integer used for the example fee in dollars.
- `reward`: an enum value for cashback, reward points, or miles.
- `rate`: a `double` for the percentage, such as 1.25.
- `multipliers`: an array of ten `Multiplier` records, indexed by merchant category.
- `guid`: a Windows-only identifier retained from the original window code.

`Multiplier` is another struct. It pairs a merchant category with its bonus percentage. The category enum gives names to positions such as `AIRLINES`, `STORES`, and `SERVICES`. `addMultiplier` writes to the matching position in the array.

`registerCard` begins with a zero-initialized record. That leaves unassigned bonuses at zero. On Windows it also attempts to make the GUID. The GUID is not used as the saved wallet ID.

## Keeping IDs consistent

The older `testGetCard` function uses positions 0 through 4. The menu and saved file use IDs 1 through 5. `getCardById` checks the public ID first, then subtracts 1 before calling `testGetCard`.

This check matters because the older function returns the first card for an unknown position. That fallback must not make a damaged saved ID look like Travel Sample. `getCardById(0)` and `getCardById(6)` return `NULL` instead.

Removing ID 1 from a wallet does not rename ID 3 to ID 1. The ID belongs to the catalog record, not its position in the wallet. A saved file containing 3 should always load Cashback Sample in this version.

## Showing the data

`printSampleCards` prints all five names, example rates, fees, and category bonus percentages. It also prints the made-up-data warning. `getBaseInfo` and `getMultiplierInfo` supply the shorter Windows detail labels. Their static text buffers are copied into controls by `SetWindowTextW`.

The catalog therefore supplies the values, while Oliver's menu and window decide where to show them. Eden's saved file stores only IDs. It does not store another set of rates that could disagree with the catalog.

## The existing practice calculation

The old `calculateReward` function is still present:

```text
base reward = amount × (base rate / 100)
reward = base reward × (1 + bonus percentage / 100)
```

For an amount of 10, Travel Sample's 1.25% base rate gives 0.125 reward units. Its services bonus is 2%, so the result is `0.125 × 1.02 = 0.1275` units. A +5% bonus means multiply the base reward by 1.05, not by 5.

This is a practice formula. It does not compare the cash value of points and miles, subtract annual fees, check real benefit limits, or round cashback in whole cents. It should not be presented as the finished purchase calculator.

## Checks and next work

`testModules.c` checks the five ID mappings, all five names, base rates, fees, and the three bonus values per card. It also checks invalid IDs and three practice calculation cases. The recorded runs passed those checks. The tests are assigned to Evan; the expected catalog values belong in Joe's area.

Gate 4 still needs benefit sources and check dates, purchase examples, eligible-category comparisons, and important restrictions. Real data needs a source rather than just replacing a sample name. Ties, activation rules, limits, and expiration dates also need to be shown when they apply.
