package org.deeplearning4j.linalg.jcublas;

import org.deeplearning4j.linalg.api.complex.IComplexDouble;
import org.deeplearning4j.linalg.jcublas.complex.JCublasComplexNDArray;

/**
 * Created by mjk on 8/20/14.
 */
public class JCublasWrapper implements org.deeplearning4j.linalg.factory.BlasWrapper<JCublasNDArray,JCublasComplexNDArray,IComplexDouble> {
    @Override
    public JCublasNDArray swap(JCublasNDArray x, JCublasNDArray y) {
        return null;
    }

    @Override
    public JCublasNDArray scal(double alpha, JCublasNDArray x) {
        return null;
    }

    @Override
    public JCublasComplexNDArray scal(IComplexDouble alpha, JCublasComplexNDArray x) {
        return null;
    }

    @Override
    public JCublasNDArray copy(JCublasNDArray x, JCublasNDArray y) {
        return null;
    }

    @Override
    public JCublasComplexNDArray copy(JCublasComplexNDArray x, JCublasComplexNDArray y) {
        return null;
    }

    @Override
    public JCublasNDArray axpy(double da, JCublasNDArray dx, JCublasNDArray dy) {
        return null;
    }

    @Override
    public JCublasComplexNDArray axpy(IComplexDouble da, JCublasComplexNDArray dx, JCublasComplexNDArray dy) {
        return null;
    }

    @Override
    public double dot(JCublasNDArray x, JCublasNDArray y) {
        return 0;
    }

    @Override
    public IComplexDouble dotc(JCublasComplexNDArray x, JCublasComplexNDArray y) {
        return null;
    }

    @Override
    public IComplexDouble dotu(JCublasComplexNDArray x, JCublasComplexNDArray y) {
        return null;
    }

    @Override
    public double nrm2(JCublasNDArray x) {
        return 0;
    }

    @Override
    public double nrm2(JCublasComplexNDArray x) {
        return 0;
    }

    @Override
    public double asum(JCublasNDArray x) {
        return 0;
    }

    @Override
    public double asum(JCublasComplexNDArray x) {
        return 0;
    }

    @Override
    public int iamax(JCublasNDArray x) {
        return 0;
    }

    @Override
    public int iamax(JCublasComplexNDArray x) {
        return 0;
    }

    @Override
    public JCublasNDArray gemv(double alpha, JCublasNDArray a, JCublasNDArray b, double beta, JCublasNDArray c) {
        c = SimpleJCublas.gemm(a,b, alpha, beta);
        return c;
    }

    @Override
    public JCublasNDArray ger(double alpha, JCublasNDArray x, JCublasNDArray y, JCublasNDArray a) {
        return null;
    }

    @Override
    public JCublasComplexNDArray geru(IComplexDouble alpha, JCublasComplexNDArray x, JCublasComplexNDArray y, JCublasComplexNDArray a) {
        return null;
    }

    @Override
    public JCublasComplexNDArray gerc(IComplexDouble alpha, JCublasComplexNDArray x, JCublasComplexNDArray y, JCublasComplexNDArray a) {
        return null;
    }

    @Override
    public JCublasNDArray gemm(double alpha, JCublasNDArray a, JCublasNDArray b, double beta, JCublasNDArray c) {
        c = SimpleJCublas.gemm(a,b, alpha, beta);
        return c;
    }

    @Override
    public JCublasComplexNDArray gemm(IComplexDouble alpha, JCublasComplexNDArray a, JCublasComplexNDArray b, IComplexDouble beta, JCublasComplexNDArray c) {
        return null;
    }

    @Override
    public JCublasNDArray gesv(JCublasNDArray a, int[] ipiv, JCublasNDArray b) {
        return null;
    }

    @Override
    public void checkInfo(String name, int info) {

    }

    @Override
    public JCublasNDArray sysv(char uplo, JCublasNDArray a, int[] ipiv, JCublasNDArray b) {
        return null;
    }

    @Override
    public int syev(char jobz, char uplo, JCublasNDArray a, JCublasNDArray w) {
        return 0;
    }

    @Override
    public int syevx(char jobz, char range, char uplo, JCublasNDArray a, double vl, double vu, int il, int iu, double abstol, JCublasNDArray w, JCublasNDArray z) {
        return 0;
    }

    @Override
    public int syevd(char jobz, char uplo, JCublasNDArray A, JCublasNDArray w) {
        return 0;
    }

    @Override
    public int syevr(char jobz, char range, char uplo, JCublasNDArray a, double vl, double vu, int il, int iu, double abstol, JCublasNDArray w, JCublasNDArray z, int[] isuppz) {
        return 0;
    }

    @Override
    public void posv(char uplo, JCublasNDArray A, JCublasNDArray B) {

    }

    @Override
    public int geev(char jobvl, char jobvr, JCublasNDArray A, JCublasNDArray WR, JCublasNDArray WI, JCublasNDArray VL, JCublasNDArray VR) {
        return 0;
    }

    @Override
    public int sygvd(int itype, char jobz, char uplo, JCublasNDArray A, JCublasNDArray B, JCublasNDArray W) {
        return 0;
    }

    @Override
    public void gelsd(JCublasNDArray A, JCublasNDArray B) {

    }

    @Override
    public void geqrf(JCublasNDArray A, JCublasNDArray tau) {

    }

    @Override
    public void ormqr(char side, char trans, JCublasNDArray A, JCublasNDArray tau, JCublasNDArray C) {

    }
}
