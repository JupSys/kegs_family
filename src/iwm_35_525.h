
#ifdef INCLUDE_IWM_RCSID_C
const char rcsdif_iwm_35_525_h[] = "@(#)$Header: iwm_35_525.h,v 1.3 97/09/23 00:19:14 kentd Exp $";
#endif

int
IWM_READ_ROUT (Disk *dsk, int fast_disk_emul, double dcycs)
{
	Track	*trk;
	double	dcycs_last_read;
	int	pos;
	int	pos2;
	int	size;
	int	next_size;
	int	qtr_track;
	int	skip_nibs;
	int	track_len;
	byte	ret;
	int	shift;
	int	skip;
	int	cycs_this_nib;
	int	cycs_passed;
	double	dcycs_this_nib;
	double	dcycs_next_nib;
	double	dcycs_passed;
	double	track_dcycs;
	double	dtmp;

	iwm.previous_write_bits = -1;

	qtr_track = dsk->cur_qtr_track;

	trk = &(dsk->tracks[qtr_track]);
	track_len = trk->track_len;

	dcycs_last_read = dsk->dcycs_last_read;
	dcycs_passed = dcycs - dcycs_last_read;

	pos = dsk->nib_pos;

	cycs_passed = (int)dcycs_passed;

	if(track_len == 0) {
		return (cycs_passed & 0x7f) + 0x80;
	}
	size = trk->nib_area[pos];

	while(size == 0) {
		pos += 2;
		if(pos >= track_len) {
			pos = 0;
		}
		size = trk->nib_area[pos];
	}

	cycs_this_nib = size * (2 * IWM_CYC_MULT);
	dcycs_this_nib = (double)cycs_this_nib;

	if(fast_disk_emul) {
		cycs_passed = cycs_this_nib;
		dcycs_passed = dcycs_this_nib;
		/* pull a trick to make disk motor-on test pass ($bd34 RWTS) */
		/*  if this would be a sync byte, and we didn't just do this */
		/*  then don't return whole byte */
		if(size > 8 && (g_iwm_fake_fast == 0)) {
			cycs_passed = cycs_passed >> 1;
			dcycs_passed = dcycs_passed * 0.5;
			g_iwm_fake_fast = 1;
		} else {
			g_iwm_fake_fast = 0;
		}
	}

	skip = 0;
	if(cycs_passed >= (cycs_this_nib + 11)) {
		/* skip some bits? */
		skip = 1;
		if(iwm.iwm_mode & 1) {
			/* latch mode */

			pos2 = pos + 2;
			if(pos2 >= track_len) {
				pos2 = 0;
			}
			next_size = trk->nib_area[pos2];
			while(next_size == 0) {
				pos2 += 2;
				if(pos2 >= track_len) {
					pos2 = 0;
				}
				next_size = trk->nib_area[pos2];
			}

			dcycs_next_nib = next_size * (2 * IWM_CYC_MULT);

			if(dcycs_passed < (dcycs_this_nib + dcycs_next_nib)) {
				skip = 0;
			}
		}
	}

	if(skip) {
		iwm_printf("skip since cycs_passed: %f, cycs_this_nib: %f\n",
			dcycs_passed, dcycs_this_nib);

		track_dcycs = IWM_CYC_MULT * (track_len * 8);

		if(dcycs_passed >= track_dcycs) {
			dtmp = (int)(dcycs_passed / track_dcycs);
			dcycs_passed = dcycs_passed -
					(dtmp * track_dcycs);
			dcycs_last_read += (dtmp * track_dcycs);
		}

		if(dcycs_passed >= track_dcycs || dcycs_passed < 0.0) {
			dcycs_passed = 0.0;
		}

		cycs_passed = (int)dcycs_passed;

		skip_nibs = ((word32)cycs_passed) >> (4 + IWM_DISK_525);

		pos += skip_nibs * 2;
		while(pos >= track_len) {
			pos -= track_len;
		}

		dcycs_last_read += (skip_nibs * 16 * IWM_CYC_MULT);

		dsk->dcycs_last_read = dcycs_last_read;

		size = trk->nib_area[pos];
		dcycs_passed = dcycs - dcycs_last_read;
		if(dcycs_passed < 0.0 || dcycs_passed > 64.0) {
			printf("skip, last_read:%f, dcycs:%f, dcycs_pass:%f\n",
				dcycs_last_read, dcycs, dcycs_passed);
			set_halt(1);
		}

		while(size == 0) {
			pos += 2;
			if(pos >= track_len) {
				pos = 0;
			}
			size = trk->nib_area[pos];
		}

		cycs_this_nib = size * (2 * IWM_CYC_MULT);
		cycs_passed = (int)dcycs_passed;
		dcycs_this_nib = cycs_this_nib;
	}

	if(cycs_passed < cycs_this_nib) {
		/* partial */
#if 0
		iwm_printf("Disk partial, %f < %f, size: %d\n",
					dcycs_passed, dcycs_this_nib, size);
#endif
		shift = (cycs_passed) >> (1 + IWM_DISK_525);
		ret = trk->nib_area[pos+1] >> (size - shift);
		if(ret & 0x80) {
			printf("Bad shift in partial read: %02x, but "
				"c_pass:%f, this_nib:%f, shift: %d, size: %d\n",
				ret, dcycs_passed, dcycs_this_nib, shift, size);
			set_halt(1);
		}
	} else {
		/* whole thing */
		ret = trk->nib_area[pos+1];
		pos += 2;
		if(pos >= track_len) {
			pos = 0;
		}
		if(!fast_disk_emul) {
			dsk->dcycs_last_read = dcycs_last_read + dcycs_this_nib;
		}
	}

	dsk->nib_pos = pos;

#if 0
	iwm_printf("Disk read, returning: %02x\n", ret);
#endif

	return ret;
}


void
IWM_WRITE_ROUT (Disk *dsk, word32 val, int fast_disk_emul, double dcycs)
{
	double	dcycs_last_read;
	word32	bits_read;
	word32	mask;
	word32	prev_val;
	double	dcycs_this_nib;
	double	dcycs_passed;
	int	sdiff;
	int	prev_bits;

	if(dsk->fd < 0) {
		printf("Tried to write to type: %d, drive: %d, fd: %d!\n",
			IWM_DISK_525, dsk->drive, dsk->fd);
		set_halt(1);
		return;
	}

	dcycs_last_read = dsk->dcycs_last_read;

	dcycs_passed = dcycs - dcycs_last_read;

	prev_val = iwm.previous_write_val;
	prev_bits = iwm.previous_write_bits;
	mask = 0x100;
	iwm_printf("Iwm write: prev: %x,%d, new:%02x\n", prev_val, prev_bits,
							val);

	if(iwm.iwm_mode & 2) {
		/* async mode = 3.5" default */
		bits_read = 8;
	} else {
		/* sync mode, 5.25" drives */
		bits_read = ((int)dcycs_passed) >> (1 + IWM_DISK_525);
		if(bits_read < 8) {
			bits_read = 8;
		}
	}

	if(fast_disk_emul) {
		bits_read = 8;
	}
	
	dcycs_this_nib = bits_read * (2 * IWM_CYC_MULT);

	if(fast_disk_emul) {
		dcycs_passed = dcycs_this_nib;
	}

	if(prev_bits > 0) {
		while((prev_val & 0x80) == 0 && bits_read > 0) {
			/* previous byte needs some bits */
			mask = mask >> 1;
			prev_val = (prev_val << 1) + ((val & mask) !=0);
			prev_bits++;
			bits_read--;
		}
	}

	val = val & (mask - 1);
	if(prev_bits > 0) {
		/* always force out prev_val if it had any bits before */
		/*  this prevents writes of 0 from messing us up */
		iwm.previous_write_val = val;
		iwm.previous_write_bits = bits_read;

		iwm_printf("iwm_write: prev: %02x, %d, left:%02x, %d\n",
			prev_val, prev_bits, val, bits_read);
		disk_nib_out(dsk, prev_val, prev_bits);
	} else if(val & 0x80) {
		iwm_printf("iwm_write: new: %02x, %d\n", val,bits_read);
		disk_nib_out(dsk, val, bits_read);
		bits_read = 0;
	} else {
		iwm_printf("iwm_write: zip: %02x, %d, left:%02x,%d\n",
			prev_val, prev_bits, val,bits_read);
	}

	iwm.previous_write_val = val;
	iwm.previous_write_bits = bits_read;

	sdiff = dcycs - dcycs_last_read;
	if(sdiff < (dcycs_this_nib) || (sdiff > (2*dcycs_this_nib)) ) {
		dsk->dcycs_last_read = dcycs;
	} else {
		dsk->dcycs_last_read = dcycs_last_read + dcycs_this_nib;
	}
}
