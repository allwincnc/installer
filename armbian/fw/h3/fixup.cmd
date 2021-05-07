# ------------------------------------------------------------
# --- this script will upload and run the ARISC firmware ---
# ------------------------------------------------------------

setenv ARISC_blob_name "arisc-fw.code"

# this data valid for Allwinner H2+/H3/H5 SoCs
setenv tmp_addr 0x44001000
setenv SRAM_A2_addr 0x00040000
setenv R_CPUCFG_addr 0x01F01C00
setenv fw_max_size 0xC000

if test -e ${devtype} ${devnum} ${prefix}${ARISC_blob_name}; then
	echo "Found the ARISC firmware blob `${prefix}${ARISC_blob_name}`"
	base 0x0
	echo "  asserting the ARISC core reset.."
	mw.l ${R_CPUCFG_addr} 0x0 0x1
	echo "  asserting reset done, trying to upload the blob to the SRAM A2 (${SRAM_A2_addr}).."
	load ${devtype} ${devnum} ${tmp_addr} ${prefix}${ARISC_blob_name}
	cp.b ${tmp_addr} ${SRAM_A2_addr} ${fw_max_size}
	echo "  uploading done, de-asserting the ARISC core reset.."
	mw.l ${R_CPUCFG_addr} 0x1 0x1
	sleep 0.1
	if itest.l "*${R_CPUCFG_addr}" == "0x00000001"; then
		echo "  de-asserting reset done, the ARISC core is ON."
	else
		echo "  error while de-asserting reset, the ARISC core is OFF."
	fi
fi

# Recompile with:
# mkimage -C none -A arm -T script -d /boot/fixup.cmd /boot/fixup.scr
