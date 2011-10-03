/*
  Teem: Tools to process and visualize scientific data and images              
  Copyright (C) 2011, 2010, 2009  University of Chicago
  Copyright (C) 2008, 2007, 2006, 2005  Gordon Kindlmann
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public License
  (LGPL) as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  The terms of redistributing and/or modifying this software also
  include exceptions to the LGPL that facilitate static linking.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "nrrd.h"
#include "privateNrrd.h"

NrrdDeringContext *
nrrdDeringContextNew(void) {
  NrrdDeringContext *drc;
  unsigned int pi;

  drc = AIR_CALLOC(1, NrrdDeringContext);
  if (!drc) {
    return NULL;
  }
  drc->verbose = 0;
  drc->linearInterp = AIR_FALSE;
  drc->nin = NULL;
  drc->center[0] = AIR_NAN;
  drc->center[1] = AIR_NAN;
  drc->radiusScale = 1.0;
  drc->thetaNum = 0;
  drc->rkernel = NULL;
  drc->tkernel = NULL;
  for (pi=0; pi<NRRD_KERNEL_PARMS_NUM; pi++) {
    drc->rkparm[pi] = drc->tkparm[pi] = AIR_NAN;
  }
  
  return drc;
}

NrrdDeringContext *
nrrdDeringContextNix(NrrdDeringContext *drc) {

  if (drc) {
    free(drc);
  }
  return NULL;
}

int
nrrdDeringVerboseSet(NrrdDeringContext *drc, int verbose) {
  static const char me[]="nrrdDeringVerboseSet";

  if (!drc) {
    biffAddf(NRRD, "%s: got NULL pointer", me);
    return 1;
  }
  
  drc->verbose = verbose;
  return 0;
}

int
nrrdDeringLinearInterpSet(NrrdDeringContext *drc, int linterp) {
  static const char me[]="nrrdDeringLinearInterpSet";

  if (!drc) {
    biffAddf(NRRD, "%s: got NULL pointer", me);
    return 1;
  }
  
  drc->linearInterp = linterp;
  return 0;
}

int
nrrdDeringInputSet(NrrdDeringContext *drc, const Nrrd *nin) {
  static const char me[]="nrrdDeringInputSet";
  
  if (!( drc && nin )) {
    biffAddf(NRRD, "%s: got NULL pointer", me);
    return 1;
  }
  if (nrrdCheck(nin)) {
    biffAddf(NRRD, "%s: problems with given nrrd", me);
    return 1;
  }
  if (nrrdTypeBlock == nin->type) {
    biffAddf(NRRD, "%s: can't resample from type %s", me,
             airEnumStr(nrrdType, nrrdTypeBlock));
    return 1;
  }
  if (!( 2 == nin->dim || 3 == nin->dim )) {
    biffAddf(NRRD, "%s: need 2 or 3 dim nrrd (not %u)", me, nin->dim);
    return 1;
  }

  if (drc->verbose > 2) {
    fprintf(stderr, "%s: hi\n", me);
  }
  drc->nin = nin;
  drc->cdata = AIR_CAST(const char *, nin->data);
  drc->sliceSize = (nin->axis[0].size
                    * nin->axis[1].size
                    * nrrdElementSize(nin));
  if (drc->verbose > 2) {
    fprintf(stderr, "%s: sliceSize = %u\n", me,
            AIR_CAST(unsigned int, drc->sliceSize));
  }

  return 0;
}

int
nrrdDeringCenterSet(NrrdDeringContext *drc, double cx, double cy) {
  static const char me[]="nrrdDeringCenterSet";

  if (!drc) {
    biffAddf(NRRD, "%s: got NULL pointer", me);
    return 1;
  }
  if (!( AIR_EXISTS(cx) && AIR_EXISTS(cy) )) {
    biffAddf(NRRD, "%s: center (%g,%g) doesn't exist", me, cx, cy);
    return 1;
  }
  
  drc->center[0] = cx;
  drc->center[1] = cy;

  return 0;
}

int
nrrdDeringRadiusScaleSet(NrrdDeringContext *drc, double rsc) {
  static const char me[]="nrrdDeringRadiusScaleSet";

  if (!drc) {
    biffAddf(NRRD, "%s: got NULL pointer", me);
    return 1;
  }
  if (!( AIR_EXISTS(rsc) && rsc > 0.0 )) {
    biffAddf(NRRD, "%s: need finite positive radius scale, not %g", me, rsc);
    return 1;
  }

  drc->radiusScale = rsc;
  
  return 0;
}

int
nrrdDeringThetaNumSet(NrrdDeringContext *drc, unsigned int thetaNum) {
  static const char me[]="nrrdDeringThetaNumSet";

  if (!drc) {
    biffAddf(NRRD, "%s: got NULL pointer", me);
    return 1;
  }
  if (!thetaNum) {
    biffAddf(NRRD, "%s: need non-zero thetaNum", me);
    return 1;
  }
  
  drc->thetaNum = thetaNum;

  return 0;
}

int
nrrdDeringRadialKernelSet(NrrdDeringContext *drc,
                          const NrrdKernel *rkernel,
                          const double rkparm[NRRD_KERNEL_PARMS_NUM]) {
  static const char me[]="nrrdDeringRadialKernelSet";
  unsigned int pi;

  if (!( drc && rkernel && rkparm )) {
    biffAddf(NRRD, "%s: got NULL pointer", me);
    return 1;
  }

  drc->rkernel = rkernel;
  for (pi=0; pi<NRRD_KERNEL_PARMS_NUM; pi++) {
    drc->rkparm[pi] = rkparm[pi];
  }

  return 0;
}

int
nrrdDeringThetaKernelSet(NrrdDeringContext *drc,
                         const NrrdKernel *tkernel,
                         const double tkparm[NRRD_KERNEL_PARMS_NUM]) {
  static const char me[]="nrrdDeringThetaKernelSet";
  unsigned int pi;

  if (!( drc && tkernel && tkparm )) {
    biffAddf(NRRD, "%s: got NULL pointer", me);
    return 1;
  }

  drc->tkernel = tkernel;
  for (pi=0; pi<NRRD_KERNEL_PARMS_NUM; pi++) {
    drc->tkparm[pi] = tkparm[pi];
  }

  return 0;
}

/*
** per-thread state for deringing
*/
#define PTXF_NUM 5
#define ORIG 0
#define WGHT 1
#define BLRR 2
#define DIFF 3
#define RING 4
typedef struct {
  unsigned int zi;
  double radMax;
  size_t radNum;
  airArray *mop;
  Nrrd *nsliceOrig,         /* wrapped slice of nin, sneakily non-const */
    *nslice,                /* slice of nin, converted to double */
    *nptxf[PTXF_NUM];
  double *slice, *ptxf, *wght, *ring;
  NrrdResampleContext *rsmc[2];
} deringBag;

static deringBag *
deringBagNew(NrrdDeringContext *drc, double radMax) {
  deringBag *dbg;
  unsigned int pi;

  dbg = AIR_CALLOC(1, deringBag);
  dbg->radMax = radMax;
  dbg->radNum = AIR_ROUNDUP(drc->radiusScale*radMax);

  dbg->mop = airMopNew();
  dbg->nsliceOrig = nrrdNew();
  airMopAdd(dbg->mop, dbg->nsliceOrig, (airMopper)nrrdNix /* not Nuke! */,
            airMopAlways);
  dbg->nslice = nrrdNew();
  airMopAdd(dbg->mop, dbg->nslice, (airMopper)nrrdNuke, airMopAlways);
  for (pi=0; pi<PTXF_NUM; pi++) {
    dbg->nptxf[pi] = nrrdNew();
    airMopAdd(dbg->mop, dbg->nptxf[pi], (airMopper)nrrdNuke, airMopAlways);
  }

  dbg->rsmc[0] = nrrdResampleContextNew();
  airMopAdd(dbg->mop, dbg->rsmc[0], (airMopper)nrrdResampleContextNix,
            airMopAlways);
  dbg->rsmc[1] = nrrdResampleContextNew();
  airMopAdd(dbg->mop, dbg->rsmc[1], (airMopper)nrrdResampleContextNix,
            airMopAlways);

  return dbg;
}

static deringBag *
deringBagNix(deringBag *dbg) {

  airMopOkay(dbg->mop);
  airFree(dbg);
  return NULL;
}

static int
deringPtxfAlloc(NrrdDeringContext *drc, deringBag *dbg) {
  static const char me[]="deringPtxfAlloc";
  unsigned int pi, ri;
  int E;

  /* polar transform setup */
  for (pi=0; pi<PTXF_NUM; pi++) {
    if (nrrdMaybeAlloc_va(dbg->nptxf[pi], nrrdTypeDouble, 2,
                          dbg->radNum,
                          AIR_CAST(size_t, drc->thetaNum))) {
      biffAddf(NRRD, "%s: polar transform allocation problem", me);
      return 1;
    }
  }
  dbg->ptxf = AIR_CAST(double *, dbg->nptxf[ORIG]->data);
  dbg->wght = AIR_CAST(double *, dbg->nptxf[WGHT]->data);
  dbg->ring = AIR_CAST(double *, dbg->nptxf[RING]->data);

  E = AIR_FALSE;
  for (ri=0; ri<2; ri++) {
    char kstr[AIR_STRLEN_LARGE];
    if (0 == ri) {
      if (!E) E |= nrrdResampleInputSet(dbg->rsmc[0], dbg->nptxf[ORIG]);
      nrrdKernelSprint(kstr, drc->rkernel, drc->rkparm);
      if (!E) E |= nrrdResampleKernelSet(dbg->rsmc[0], 0,
                                         drc->rkernel, drc->rkparm);
      if (!E) E |= nrrdResampleKernelSet(dbg->rsmc[0], 1, NULL, NULL);
    } else {
      if (!E) E |= nrrdResampleInputSet(dbg->rsmc[1], dbg->nptxf[DIFF]);
      nrrdKernelSprint(kstr, drc->tkernel, drc->tkparm);
      if (!E) E |= nrrdResampleKernelSet(dbg->rsmc[1], 0, NULL, NULL);
      if (!E) E |= nrrdResampleKernelSet(dbg->rsmc[1], 1,
                                         drc->tkernel, drc->tkparm);
    }
    if (!E) E |= nrrdResampleDefaultCenterSet(dbg->rsmc[ri], nrrdCenterCell);
    if (!E) E |= nrrdResampleSamplesSet(dbg->rsmc[ri], 0, dbg->radNum);
    if (!E) E |= nrrdResampleSamplesSet(dbg->rsmc[ri], 1, drc->thetaNum);
    if (!E) E |= nrrdResampleBoundarySet(dbg->rsmc[ri], nrrdBoundaryWrap);
    if (!E) E |= nrrdResampleTypeOutSet(dbg->rsmc[ri], nrrdTypeDefault);
    if (!E) E |= nrrdResampleRenormalizeSet(dbg->rsmc[ri], AIR_TRUE);
    if (!E) E |= nrrdResampleNonExistentSet(dbg->rsmc[ri],
                                            nrrdResampleNonExistentRenormalize);
    if (!E) E |= nrrdResampleRangeFullSet(dbg->rsmc[ri], 0);
    if (!E) E |= nrrdResampleRangeFullSet(dbg->rsmc[ri], 1);
  }
  if (E) {
    biffAddf(NRRD, "%s: couldn't set up resampler", me);
    return 1;
  }
  

  return 0;
}

static int
deringSliceGet(NrrdDeringContext *drc, deringBag *dbg, unsigned int zi) {
  static const char me[]="deringSliceGet";

  /* slice setup */
  if (nrrdWrap_va(dbg->nsliceOrig,
                  /* HEY: sneaky bypass of const-ness of drc->cdata */
                  AIR_CAST(void *, drc->cdata + zi*(drc->sliceSize)),
                  drc->nin->type, 2, 
                  drc->nin->axis[0].size,
                  drc->nin->axis[1].size)
      || (nrrdTypeDouble == drc->nin->type
          ? nrrdCopy(dbg->nslice, dbg->nsliceOrig)
          : nrrdConvert(dbg->nslice, dbg->nsliceOrig, nrrdTypeDouble))) {
    biffAddf(NRRD, "%s: slice setup trouble", me);
    return 1;
  }
  dbg->slice = AIR_CAST(double *, dbg->nslice->data);
  dbg->zi = zi;
  
  return 0;
}

#define EPS 0.000001

static void
deringXYtoRT(NrrdDeringContext *drc, deringBag *dbg,
             unsigned int xi, unsigned int yi,
             unsigned int *rrIdx, unsigned int *thIdx,
             double *rrFrc, double *thFrc) {
  double dx, dy, rr, th, rrScl, thScl;
  dx = xi - drc->center[0];
  dy = yi - drc->center[1];
  rr = sqrt(dx*dx + dy*dy);
  th = atan2(-dx, dy);
  rrScl = AIR_AFFINE(-EPS, rr, dbg->radMax+EPS, 0.0, dbg->radNum-1);
  *rrIdx = AIR_CAST(unsigned int, 0.5 + rrScl);
  thScl = AIR_AFFINE(-AIR_PI-EPS, th, AIR_PI+EPS, 0.0, drc->thetaNum-1);
  *thIdx = AIR_CAST(unsigned int, 0.5 + thScl);
  if (rrFrc && thFrc) {
    *rrFrc = rrScl - *rrIdx;
    *thFrc = thScl - *thIdx;
    if (*rrFrc < 0) {
      *rrIdx -= 1;
      *rrFrc += 1;
    }
    if (*thFrc < 0) {
      *thIdx -= 1;
      *thFrc += 1;
    }
  }
  return;
}

static int
deringPtxfDo(NrrdDeringContext *drc, deringBag *dbg) {
  /* static const char me[]="deringPtxfDo"; */
  unsigned int sx, sy, xi, yi, rrIdx, thIdx;

  nrrdZeroSet(dbg->nptxf[ORIG]);
  nrrdZeroSet(dbg->nptxf[WGHT]);
  sx = AIR_CAST(unsigned int, drc->nin->axis[0].size);
  sy = AIR_CAST(unsigned int, drc->nin->axis[1].size);
  for (yi=0; yi<sy; yi++) {
    for (xi=0; xi<sx; xi++) {
      double rrFrc, thFrc, val;
      if (drc->linearInterp) {
        unsigned int bidx;
        deringXYtoRT(drc, dbg, xi, yi, &rrIdx, &thIdx, &rrFrc, &thFrc);
        bidx = rrIdx + dbg->radNum*thIdx;
        val = dbg->slice[xi + sx*yi];
        dbg->ptxf[bidx                  ] += (1-rrFrc)*(1-thFrc)*val;
        dbg->ptxf[bidx + 1              ] +=     rrFrc*(1-thFrc)*val;
        dbg->ptxf[bidx     + dbg->radNum] += (1-rrFrc)*thFrc*val;
        dbg->ptxf[bidx + 1 + dbg->radNum] +=     rrFrc*thFrc*val;
        dbg->wght[bidx                  ] += (1-rrFrc)*(1-thFrc);
        dbg->wght[bidx + 1              ] +=     rrFrc*(1-thFrc);
        dbg->wght[bidx     + dbg->radNum] += (1-rrFrc)*thFrc;
        dbg->wght[bidx + 1 + dbg->radNum] +=     rrFrc*thFrc;
      } else {
        deringXYtoRT(drc, dbg, xi, yi, &rrIdx, &thIdx, NULL, NULL);
        dbg->ptxf[rrIdx + dbg->radNum*thIdx] += dbg->slice[xi + sx*yi];
        dbg->wght[rrIdx + dbg->radNum*thIdx] += 1;
      }
    }
  }
  for (thIdx=0; thIdx<drc->thetaNum; thIdx++) {
    for (rrIdx=0; rrIdx<dbg->radNum; rrIdx++) {
      double tmpW;
      tmpW = dbg->wght[rrIdx + dbg->radNum*thIdx];
      if (tmpW) {
        dbg->ptxf[rrIdx + dbg->radNum*thIdx] /= tmpW;
      } else {
        dbg->ptxf[rrIdx + dbg->radNum*thIdx] = AIR_NAN;
      }
    }
  }
  if (0) {
    char fname[AIR_STRLEN_SMALL];
    sprintf(fname, "wght-%02u.nrrd", dbg->zi);
    nrrdSave(fname, dbg->nptxf[WGHT], NULL);
  }

  return 0;
}

static int
deringPtxfFilter(NrrdDeringContext *drc, deringBag *dbg) {
  static const char me[]="deringPtxfFilter";

  AIR_UNUSED(drc);
  if (nrrdResampleExecute(dbg->rsmc[0], dbg->nptxf[BLRR])
      || nrrdArithBinaryOp(dbg->nptxf[DIFF], nrrdBinaryOpSubtract,
                           dbg->nptxf[ORIG], dbg->nptxf[BLRR])
      || nrrdResampleExecute(dbg->rsmc[1], dbg->nptxf[RING])) {
    biffAddf(NRRD, "%s: trouble", me);
    return 1;
  }
  if (0) {
    char fname[AIR_STRLEN_SMALL];
    sprintf(fname, "orig-%02u.nrrd", dbg->zi);
    nrrdSave(fname, dbg->nptxf[ORIG], NULL);
    sprintf(fname, "blrr-%02u.nrrd", dbg->zi);
    nrrdSave(fname, dbg->nptxf[BLRR], NULL);
    sprintf(fname, "diff-%02u.nrrd", dbg->zi);
    nrrdSave(fname, dbg->nptxf[DIFF], NULL);
    sprintf(fname, "ring-%02u.nrrd", dbg->zi);
    nrrdSave(fname, dbg->nptxf[RING], NULL);
  }

  return 0;
}

static int
deringSubtract(NrrdDeringContext *drc, deringBag *dbg) {
  /* static const char me[]="deringSubtract"; */
  unsigned int sx, sy, xi, yi, rrIdx, thIdx;

  sx = AIR_CAST(unsigned int, drc->nin->axis[0].size);
  sy = AIR_CAST(unsigned int, drc->nin->axis[1].size);
  for (yi=0; yi<sy; yi++) {
    for (xi=0; xi<sx; xi++) {
      double rrFrc, thFrc, val;
      if (drc->linearInterp) {
        unsigned int bidx;
        deringXYtoRT(drc, dbg, xi, yi, &rrIdx, &thIdx, &rrFrc, &thFrc);
        bidx = rrIdx + dbg->radNum*thIdx;
        val = (dbg->ring[bidx                  ]*(1-rrFrc)*(1-thFrc) +
               dbg->ring[bidx + 1              ]*rrFrc*(1-thFrc) +
               dbg->ring[bidx     + dbg->radNum]*(1-rrFrc)*thFrc +
               dbg->ring[bidx + 1 + dbg->radNum]*rrFrc*thFrc);
        dbg->slice[xi + sx*yi] -= val;
      } else {
        deringXYtoRT(drc, dbg, xi, yi, &rrIdx, &thIdx, NULL, NULL);
        dbg->slice[xi + sx*yi] -= dbg->ring[rrIdx + dbg->radNum*thIdx];
      }
    }
  }
  if (0) {
    char fname[AIR_STRLEN_SMALL];
    sprintf(fname, "drng-%02u.nrrd", dbg->zi);
    nrrdSave(fname, dbg->nslice, NULL);
  }
  return 0;
}

static int
deringDo(NrrdDeringContext *drc, deringBag *dbg,
         Nrrd *nout, unsigned int zi) {
  static const char me[]="deringDo";

  if (deringSliceGet(drc, dbg, zi)
      || deringPtxfDo(drc, dbg)
      || deringPtxfFilter(drc, dbg)
      || deringSubtract(drc, dbg)) {
    biffAddf(NRRD, "%s: trouble", me);
    return 1;
  }
  /* HEY: convert/copy nslice to output slice */
  AIR_UNUSED(nout);

  return 0;
}

static int
deringCheck(NrrdDeringContext *drc) {
  static const char me[]="deringCheck";

  if (!(drc->nin)) {
    biffAddf(NRRD, "%s: no input set", me);
    return 1;
  }
  if (!( AIR_EXISTS(drc->center[0]) && AIR_EXISTS(drc->center[1]) )) {
    biffAddf(NRRD, "%s: no center set", me);
    return 1;
  }
  if (!(drc->thetaNum)) {
    biffAddf(NRRD, "%s: no thetaNum set", me);
    return 1;
  }
  if (!( drc->rkernel && drc->tkernel )) {
    biffAddf(NRRD, "%s: R and T kernels not both set", me);
    return 1;
  }
  return 0;
}

int
nrrdDeringExecute(NrrdDeringContext *drc, Nrrd *nout) {
  static const char me[]="nrrdDeringExecute";
  unsigned int sx, sy, sz, zi;
  double dx, dy, radLen, len;
  deringBag *dbg;
  airArray *mop;
  
  if (!( drc && nout )) {
    biffAddf(NRRD, "%s: got NULL pointer", me);
    return 1;
  }
  if (deringCheck(drc)) {
    biffAddf(NRRD, "%s: trouble with setup", me);
    return 1;
  }

  if (nrrdCopy(nout, drc->nin)) {
    biffAddf(NRRD, "%s: trouble initializing output with input", me);
    return 1;
  }
  /* set radLen: radial length of polar transform of data */
  radLen = 0;
  sx = AIR_CAST(unsigned int, drc->nin->axis[0].size);
  sy = AIR_CAST(unsigned int, drc->nin->axis[1].size);
  dx = 0 - drc->center[0];
  dy = 0 - drc->center[1];
  len = sqrt(dx*dx + dy*dy);
  radLen = AIR_MAX(radLen, len);
  dx = sx-1 - drc->center[0];
  dy = 0 - drc->center[1];
  len = sqrt(dx*dx + dy*dy);
  radLen = AIR_MAX(radLen, len);
  dx = sx-1 - drc->center[0];
  dy = sy-1 - drc->center[1];
  len = sqrt(dx*dx + dy*dy);
  radLen = AIR_MAX(radLen, len);
  dx = 0 - drc->center[0];
  dy = sy-1 - drc->center[1];
  len = sqrt(dx*dx + dy*dy);
  radLen = AIR_MAX(radLen, len);
  if (drc->verbose) {
    fprintf(stderr, "%s: radLen = %g\n", me, radLen);
  }

  /* create deringBag(s) */
  mop = airMopNew();
  dbg = deringBagNew(drc, radLen);
  airMopAdd(mop, dbg, (airMopper)deringBagNix, airMopAlways);

  if (deringPtxfAlloc(drc, dbg)) {
    biffAddf(NRRD, "%s: trouble on setup", me);
    return 1;
  }

  /* HEY: do histogram analysis to find safe clamping values */
  
  sz = (2 == drc->nin->dim
        ? 1
        : AIR_CAST(unsigned int, drc->nin->axis[2].size));
  for (zi=0; zi<sz; zi++) {
    if (drc->verbose) {
      fprintf(stderr, "%s: slice %u of %u ...\n", me, zi, sz);
    }
    if (deringDo(drc, dbg, nout, zi)) {
      biffAddf(NRRD, "%s: trouble on slice %u", me, zi);
      return 1;
    }
    if (drc->verbose) {
      fprintf(stderr, "%s: ... %u done\n", me, zi);
    }
  }

  airMopOkay(mop);
  return 0;
}