#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <sys/time.h>
#include "filepath.h"
#include "npdm.h"

void npdm_process(hbp_settings_t *settings, cnmt_ctx_t *cnmt_ctx)
{
    filepath_t npdm_filepath;
    filepath_init(&npdm_filepath);
    filepath_copy(&npdm_filepath, &settings->exefs_dir);
    filepath_append(&npdm_filepath, "main.npdm");

    FILE *fl;
    fl = os_fopen(npdm_filepath.os_path, OS_MODE_EDIT);
    if (fl == NULL)
    {
        fprintf(stderr, "Failed to open %s!\n", npdm_filepath.char_path);
        exit(EXIT_FAILURE);
    }

    // Read NPDM Header
    npdm_t npdm;
    memset(&npdm, 0, sizeof(npdm));
    if (fread(&npdm, 1, sizeof(npdm_t), fl) != sizeof(npdm_t))
    {
        fprintf(stderr, "Failed to read NPDM header!\n");
        exit(EXIT_FAILURE);
    }

    printf("Validating NPDM\n");

    // Verify NPDM magic
    if (npdm.magic != MAGIC_META)
    {
        fprintf(stderr, "Invalid NPDM magic!\n");
        exit(EXIT_FAILURE);
    }

    // Read ACID
    npdm_acid_t acid;
    fseeko64(fl, npdm.acid_offset, SEEK_SET);
    if (fread(&acid, 1, sizeof(npdm_acid_t), fl) != sizeof(npdm_acid_t))
    {
        fprintf(stderr, "Failed to read NPDM ACID!\n");
        exit(EXIT_FAILURE);
    }

    // Validate ACID MAGIC
    if (acid.magic != MAGIC_ACID)
    {
        fprintf(stderr, "Invalid ACID magic!\n");
        exit(EXIT_FAILURE);
    }

    // Read ACI0
    npdm_aci0_t aci0;
    fseeko64(fl, npdm.aci0_offset, SEEK_SET);
    if (fread(&aci0, 1, sizeof(npdm_aci0_t), fl) != sizeof(npdm_aci0_t))
    {
        fprintf(stderr, "Failed to read NPDM ACI0!\n");
        exit(EXIT_FAILURE);
    }

    // Validate ACI0 MAGIC
    if (aci0.magic != MAGIC_ACI0)
    {
        fprintf(stderr, "Invalid ACI0 magic!\n");
        exit(EXIT_FAILURE);
    }

    // Set TitleID
    printf("Getting TitleID\n");
    cnmt_ctx->cnmt_header.title_id = aci0.title_id;

    printf("Validating TitleID\n");
    // Make sure that tid is within valid range
    if (cnmt_ctx->cnmt_header.title_id < 0x0100000000000000 || cnmt_ctx->cnmt_header.title_id > 0x0fffffffffffffff)
    {
        fprintf(stderr, "Error: Bad TitleID found in main.npdm: 0x%016" PRIx64 "\n"
                        "Valid TitleID range: 0100000000000000 - 0fffffffffffffff\n",
                cnmt_ctx->cnmt_header.title_id);
        exit(EXIT_FAILURE);
    }
    if (cnmt_ctx->cnmt_header.title_id > 0x01ffffffffffffff)
        printf("Warning: TitleID %" PRIx64 " is greater than 01ffffffffffffff and it's not suggested\n", cnmt_ctx->cnmt_header.title_id);

    fclose(fl);
}