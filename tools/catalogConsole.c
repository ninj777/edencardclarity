/* A portable C entry point for the catalog console and its local checks.
   The existing Windows main also calls runCatalogApp for the new interface. */
#include "../CreditCardThing/catalogApp.h"
int main(int argc, char* argv[]) { return runCatalogApp(argc, argv); }
