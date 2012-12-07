/*
 * Copyright 2012 <James.Bottomley@HansenPartnership.com>
 *
 * see COPYING file
 *
 */

#include <efi.h>
#include <efilib.h>

#include <console.h>
#include <errors.h>
#include <security_policy.h>
#include <execute.h>

#include "hashlist.h"

CHAR16 *loader = L"\\loader.efi";
CHAR16 *hashtool = L"\\HashTool.efi";

EFI_STATUS
efi_main (EFI_HANDLE image, EFI_SYSTEM_TABLE *systab)
{
	EFI_STATUS status;

	InitializeLib(image, systab);

	status = security_policy_install();
	if (status != EFI_SUCCESS) {
		console_error(L"Failed to install override security policy",
			      status);
		return status;
	}

	/* install statically compiled in hashes */
	security_protocol_set_hashes(_tmp_tmp_hash, _tmp_tmp_hash_len);

	status = execute(image, loader);

	if (status == EFI_SUCCESS)
		goto out;

	if (status != EFI_SECURITY_VIOLATION && status != EFI_ACCESS_DENIED) {
		CHAR16 buf[256];

		StrCpy(buf, L"Failed to start ");
		StrCat(buf, loader);
		console_error(buf, status);

		goto out;
	}

	console_alertbox((CHAR16 *[]) {
			L"Failed to start loader",
			L"",
			L"It is probably in \\boot\\efi\\loader.efi",
			L"Please enrol its hash and try again",
			L"",
			L"I will now execute HashTool for you to do this",
			NULL
		});

	for (;;) {
		status = execute(image, hashtool);

		if (status != EFI_SUCCESS) {
			CHAR16 buf[256];

			StrCpy(buf, L"Failed to start backup programme ");
			StrCat(buf, hashtool);
			console_error(buf, status);

			goto out;
		}

		/* try to start the loader again */
		status = execute(image, loader);
		if (status == EFI_ACCESS_DENIED
		    || status == EFI_SECURITY_VIOLATION) {
			int selection = console_select((CHAR16 *[]) {
				L"loader is still giving a security error",
				NULL
			}, (CHAR16 *[]) {
				L"Start HashTool",
				L"Exit",
				NULL
			}, 0);
			if (selection == 0)
				continue;
		}

		break;
	}
 out:
	security_policy_uninstall();

	return status;
}