#include "jemalloc/internal/jemalloc_preamble.h"
#include "jemalloc/internal/sz.h"

JEMALLOC_ALIGNED(CACHELINE)
size_t sz_pind2sz_tab[SC_NPSIZES+1];

/*
 HIGHLIGHT sz_pind2sz_tab 数组
   所有size class中size 能整除 page的size class会记录在sz_pind2sz_tab这个map中
   value是实际的size class的size
   剩余的部分value为 所有size class 中最大的size + PAGE
 */
static void
sz_boot_pind2sz_tab(const sc_data_t *sc_data) {
	int pind = 0;
	for (unsigned i = 0; i < SC_NSIZES; i++) {
		const sc_t *sc = &sc_data->sc[i];
		if (sc->psz) {
			sz_pind2sz_tab[pind] = (ZU(1) << sc->lg_base)
			    + (ZU(sc->ndelta) << sc->lg_delta);
			pind++;
		}
	}
	for (int i = pind; i <= (int)SC_NPSIZES; i++) {
		sz_pind2sz_tab[pind] = sc_data->large_maxclass + PAGE;
	}
}

/*
 * HIGHLIGHT: 映射 index -> size class 的真实size 
 */
JEMALLOC_ALIGNED(CACHELINE)
size_t sz_index2size_tab[SC_NSIZES];

static void
sz_boot_index2size_tab(const sc_data_t *sc_data) {
	for (unsigned i = 0; i < SC_NSIZES; i++) {
		const sc_t *sc = &sc_data->sc[i];
		sz_index2size_tab[i] = (ZU(1) << sc->lg_base)
		    + (ZU(sc->ndelta) << (sc->lg_delta));
	}
}

/*
 * To keep this table small, we divide sizes by the tiny min size, which gives
 * the smallest interval for which the result can change.
 HIGHLIGHT: lookup table 所有 size 都会被放在这个 table 中.
   ((sz + (ZU(1) << SC_LG_TINY_MIN) - 1) >> SC_LG_TINY_MIN) 是一个常用的计算 round_up(sz / (1<<SC_LG_TINY_MIN)),
   例如SC_LG_TINY_MIN=3
      sz=7, ((sz + (ZU(1) << SC_LG_TINY_MIN) - 1) >> SC_LG_TINY_MIN) 为 (7+7)/8=1
      sz=8, ((sz + (ZU(1) << SC_LG_TINY_MIN) - 1) >> SC_LG_TINY_MIN) 为 (8+7)/8=1
      sz=9, ((sz + (ZU(1) << SC_LG_TINY_MIN) - 1) >> SC_LG_TINY_MIN) 为 (9+7)/8=2
   sz_size2index_tab[i] 其中下标 i 代表 size= i*(1<<SC_LG_TINY_MIN)
   值代表在size classes 数组中的下表, 这里需要确保 SC_NSIZES < 256

 */
JEMALLOC_ALIGNED(CACHELINE)
uint8_t sz_size2index_tab[(SC_LOOKUP_MAXCLASS >> SC_LG_TINY_MIN) + 1];

/*
HIGHLIGHT: sz_boot_size2index_tab 用于计算 size 到 size class的映射
 size 首先会用 round_up(sz / (1<<SC_LG_TINY_MIN)) 作为下标 i
 找到 j = sz_size2index_tab[i] 然后去 size classes 中找到对应的 size class
 */
static void
sz_boot_size2index_tab(const sc_data_t *sc_data) {
	size_t dst_max = (SC_LOOKUP_MAXCLASS >> SC_LG_TINY_MIN) + 1;
	size_t dst_ind = 0;
	for (unsigned sc_ind = 0; sc_ind < SC_NSIZES && dst_ind < dst_max;
	    sc_ind++) {
		const sc_t *sc = &sc_data->sc[sc_ind];
		size_t sz = (ZU(1) << sc->lg_base)
		    + (ZU(sc->ndelta) << sc->lg_delta);
		size_t max_ind = ((sz + (ZU(1) << SC_LG_TINY_MIN) - 1)
				   >> SC_LG_TINY_MIN);
		for (; dst_ind <= max_ind && dst_ind < dst_max; dst_ind++) {
                  //HIGHLIGHT sc_ind 必须 < 256
			sz_size2index_tab[dst_ind] = sc_ind;
		}
	}
}

void
sz_boot(const sc_data_t *sc_data) {
	sz_boot_pind2sz_tab(sc_data);
	sz_boot_index2size_tab(sc_data);
	sz_boot_size2index_tab(sc_data);
}
