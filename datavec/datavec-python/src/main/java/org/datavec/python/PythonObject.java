/*******************************************************************************
 * Copyright (c) 2020 Konduit K.K.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License, Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 ******************************************************************************/


package org.datavec.python;


import lombok.extern.slf4j.Slf4j;
import org.bytedeco.cpython.PyObject;
import org.bytedeco.javacpp.BytePointer;
import org.bytedeco.javacpp.Pointer;
import org.bytedeco.javacpp.SizeTPointer;
import org.bytedeco.numpy.PyArrayObject;
import org.json.JSONArray;
import org.json.JSONObject;
import org.nd4j.linalg.api.buffer.BaseDataBuffer;
import org.nd4j.linalg.api.buffer.DataType;
import org.nd4j.linalg.api.ndarray.INDArray;
import org.nd4j.nativeblas.NativeOpsHolder;

import java.util.*;

import static org.bytedeco.cpython.global.python.*;
import static org.bytedeco.numpy.global.numpy.*;

/**
 * Swift like python wrapper for J
 *
 * @author Fariz Rahman
 */

@Slf4j
public class PythonObject {
    private PyObject nativePythonObject;

    static {
        new PythonExecutioner();
    }

    private static Map<String, PythonObject> _getNDArraySerializer() {
        Map<String, PythonObject> ndarraySerializer = new HashMap<>();
        PythonObject lambda = Python.eval(
                "lambda x: " +
                        "{'address':" +
                        "x.__array_interface__['data'][0]," +
                        "'shape':x.shape,'strides':x.strides," +
                        "'dtype': str(x.dtype),'_is_numpy_array': True}" +
                        " if str(type(x))== \"<class 'numpy.ndarray'>\" else x");
        ndarraySerializer.put("default",
                lambda);
        return ndarraySerializer;

    }

    public PythonObject(PyObject pyObject) {
        nativePythonObject = pyObject;
    }

    public PythonObject(INDArray npArray) {
        this(new NumpyArray(npArray));
    }

    public PythonObject(BytePointer bp){

        long address = bp.address();
        long size = bp.capacity();
        NumpyArray npArr = NumpyArray.builder().address(address).shape(new long[]{size}).strides(new long[]{1}).dtype(DataType.INT8).build();
        nativePythonObject = Python.memoryview(new PythonObject(npArr)).nativePythonObject;
    }

    public PythonObject(NumpyArray npArray) {
        int numpyType;
        INDArray indArray = npArray.getNd4jArray();
        DataType dataType = indArray.dataType();

        switch (dataType) {
            case DOUBLE:
                numpyType = NPY_DOUBLE;
                break;
            case FLOAT:
            case BFLOAT16:
                numpyType = NPY_FLOAT;
                break;
            case SHORT:
                numpyType = NPY_SHORT;
                break;
            case INT:
                numpyType = NPY_INT;
                break;
            case LONG:
                numpyType = NPY_INT64;
                break;
            case UINT16:
                numpyType = NPY_USHORT;
                break;
            case UINT32:
                numpyType = NPY_UINT;
                break;
            case UINT64:
                numpyType = NPY_UINT64;
                break;
            case BOOL:
                numpyType = NPY_BOOL;
                break;
            case BYTE:
                numpyType = NPY_BYTE;
                break;
            case UBYTE:
                numpyType = NPY_UBYTE;
                break;
            case HALF:
                numpyType = NPY_HALF;
                break;
            default:
                throw new RuntimeException("Unsupported dtype: " + npArray.getDtype());
        }

        long[] shape = indArray.shape();
        INDArray inputArray = indArray;
        if(dataType == DataType.BFLOAT16) {
            log.warn("\n\nThe given nd4j array \n\n{}\n\n is of BFLOAT16 datatype. " +
                    "Casting a copy of it to FLOAT and creating the respective numpy array from it.\n", indArray);
            inputArray = indArray.castTo(DataType.FLOAT);
        }

        //Sync to host memory in the case of CUDA, before passing the host memory pointer to Python
        if(inputArray.data() instanceof BaseDataBuffer){
            ((BaseDataBuffer)inputArray.data()).syncToPrimary();
        }

        nativePythonObject = PyArray_New(PyArray_Type(), shape.length, new SizeTPointer(shape),
                numpyType, null,
                inputArray.data().addressPointer(),
                0, NPY_ARRAY_CARRAY, null);

    }

    /*---primitve constructors---*/
    public PyObject getNativePythonObject() {
        return nativePythonObject;
    }

    public PythonObject(String data) {
        nativePythonObject = PyUnicode_FromString(data);
    }

    public PythonObject(int data) {
        nativePythonObject = PyLong_FromLong((long) data);
    }

    public PythonObject(long data) {
        nativePythonObject = PyLong_FromLong(data);
    }

    public PythonObject(double data) {
        nativePythonObject = PyFloat_FromDouble(data);
    }

    public PythonObject(boolean data) {
        nativePythonObject = PyBool_FromLong(data ? 1 : 0);
    }

    private static PythonObject j2pyObject(Object item) {
        if (item instanceof PythonObject) {
            return (PythonObject) item;
        } else if (item instanceof PyObject) {
            return new PythonObject((PyObject) item);
        } else if (item instanceof INDArray) {
            return new PythonObject((INDArray) item);
        } else if (item instanceof NumpyArray) {
            return new PythonObject((NumpyArray) item);
        } else if (item instanceof List) {
            return new PythonObject((List) item);
        } else if (item instanceof Object[]) {
            return new PythonObject((Object[]) item);
        } else if (item instanceof Map) {
            return new PythonObject((Map) item);
        } else if (item instanceof String) {
            return new PythonObject((String) item);
        } else if (item instanceof Double) {
            return new PythonObject((Double) item);
        } else if (item instanceof Float) {
            return new PythonObject((Float) item);
        } else if (item instanceof Long) {
            return new PythonObject((Long) item);
        } else if (item instanceof Integer) {
            return new PythonObject((Integer) item);
        } else if (item instanceof Boolean) {
            return new PythonObject((Boolean) item);
        } else if (item instanceof Pointer){
            return new PythonObject(new BytePointer((Pointer)item));
        } else {
            throw new RuntimeException("Unsupported item in list: " + item);
        }
    }

    public PythonObject(Object[] data) {
        PyObject pyList = PyList_New((long) data.length);
        for (int i = 0; i < data.length; i++) {
            PyList_SetItem(pyList, i, j2pyObject(data[i]).nativePythonObject);
        }
        nativePythonObject = pyList;
    }

    public PythonObject(List data) {
        PyObject pyList = PyList_New((long) data.size());
        for (int i = 0; i < data.size(); i++) {
            PyList_SetItem(pyList, i, j2pyObject(data.get(i)).nativePythonObject);
        }
        nativePythonObject = pyList;
    }

    public PythonObject(Map data) {
        PyObject pyDict = PyDict_New();
        for (Object k : data.keySet()) {
            PythonObject pyKey;
            if (k instanceof PythonObject) {
                pyKey = (PythonObject) k;
            } else if (k instanceof String) {
                pyKey = new PythonObject((String) k);
            } else if (k instanceof Double) {
                pyKey = new PythonObject((Double) k);
            } else if (k instanceof Float) {
                pyKey = new PythonObject((Float) k);
            } else if (k instanceof Long) {
                pyKey = new PythonObject((Long) k);
            } else if (k instanceof Integer) {
                pyKey = new PythonObject((Integer) k);
            } else if (k instanceof Boolean) {
                pyKey = new PythonObject((Boolean) k);
            } else {
                throw new RuntimeException("Unsupported key in map: " + k.getClass());
            }
            Object v = data.get(k);
            PythonObject pyVal;
            if (v instanceof PythonObject) {
                pyVal = (PythonObject) v;
            } else if (v instanceof PyObject) {
                pyVal = new PythonObject((PyObject) v);
            } else if (v instanceof INDArray) {
                pyVal = new PythonObject((INDArray) v);
            } else if (v instanceof NumpyArray) {
                pyVal = new PythonObject((NumpyArray) v);
            } else if (v instanceof Map) {
                pyVal = new PythonObject((Map) v);
            } else if (v instanceof List) {
                pyVal = new PythonObject((List) v);
            } else if (v instanceof String) {
                pyVal = new PythonObject((String) v);
            } else if (v instanceof Double) {
                pyVal = new PythonObject((Double) v);
            } else if (v instanceof Float) {
                pyVal = new PythonObject((Float) v);
            } else if (v instanceof Long) {
                pyVal = new PythonObject((Long) v);
            } else if (v instanceof Integer) {
                pyVal = new PythonObject((Integer) v);
            } else if (v instanceof Boolean) {
                pyVal = new PythonObject((Boolean) v);
            } else {
                throw new RuntimeException("Unsupported value in map: " + k.getClass());
            }

            PyDict_SetItem(pyDict, pyKey.nativePythonObject, pyVal.nativePythonObject);

        }
        nativePythonObject = pyDict;
    }


    /*------*/

    private static String pyObjectToString(PyObject pyObject) {
        PyObject repr = PyObject_Str(pyObject);
        PyObject str = PyUnicode_AsEncodedString(repr, "utf-8", "~E~");
        String jstr = PyBytes_AsString(str).getString();
        Py_DecRef(repr);
        Py_DecRef(str);
        return jstr;
    }

    public String toString() {
        return pyObjectToString(nativePythonObject);
    }

    public double toDouble() {
        return PyFloat_AsDouble(nativePythonObject);
    }

    public float toFloat() {
        return (float) PyFloat_AsDouble(nativePythonObject);
    }

    public int toInt() {
        return (int) PyLong_AsLong(nativePythonObject);
    }

    public long toLong() {
        return PyLong_AsLong(nativePythonObject);
    }

    public boolean toBoolean() {
        if (isNone()) return false;
        return toInt() != 0;
    }

    public NumpyArray toNumpy() throws PythonException{
        PyObject np = PyImport_ImportModule("numpy");
        PyObject ndarray = PyObject_GetAttrString(np, "ndarray");
        if (PyObject_IsInstance(nativePythonObject, ndarray) != 1){
            throw new PythonException("Object is not a numpy array! Use Python.ndarray() to convert object to a numpy array.");
        }
        Py_DecRef(ndarray);
        Py_DecRef(np);

        Pointer objPtr = new Pointer(nativePythonObject);
        PyArrayObject npArr = new PyArrayObject(objPtr);
        Pointer ptr = PyArray_DATA(npArr);
        long[] shape = new long[PyArray_NDIM(npArr)];
        SizeTPointer shapePtr = PyArray_SHAPE(npArr);
        if (shapePtr != null)
            shapePtr.get(shape, 0, shape.length);
        long[] strides = new long[shape.length];
        SizeTPointer stridesPtr = PyArray_STRIDES(npArr);
        if (stridesPtr != null)
            stridesPtr.get(strides, 0, strides.length);
        int npdtype = PyArray_TYPE(npArr);

        DataType dtype;
        switch (npdtype){
            case NPY_DOUBLE:
                dtype = DataType.DOUBLE; break;
            case NPY_FLOAT:
                dtype = DataType.FLOAT; break;
            case NPY_SHORT:
                dtype = DataType.SHORT; break;
            case NPY_INT:
                dtype = DataType.INT32; break;
            case NPY_LONG:
                dtype = DataType.LONG; break;
            case NPY_UINT:
                dtype = DataType.UINT32; break;
            case NPY_BYTE:
                dtype = DataType.INT8; break;
            case NPY_UBYTE:
                dtype = DataType.UINT8; break;
            case NPY_BOOL:
                dtype = DataType.BOOL; break;
            case NPY_HALF:
                dtype = DataType.FLOAT16; break;
            case NPY_LONGLONG:
                dtype = DataType.INT64; break;
            case NPY_USHORT:
                dtype = DataType.UINT16; break;
            case NPY_ULONG:
            case NPY_ULONGLONG:
                dtype = DataType.UINT64; break;
            default:
                    throw new PythonException("Unsupported array data type: " + npdtype);
        }

        return new NumpyArray(ptr.address(), shape, strides, dtype);

    }

    public PythonObject attr(String attr) {

        return new PythonObject(PyObject_GetAttrString(nativePythonObject, attr));
    }

    public PythonObject call(Object... args) {
        if (args.length > 0 && args[args.length - 1] instanceof Map) {
            List<Object> args2 = new ArrayList<>();
            for (int i = 0; i < args.length - 1; i++) {
                args2.add(args[i]);
            }
            return call(args2, (Map) args[args.length - 1]);
        }
        if (args.length == 0) {
            return new PythonObject(PyObject_CallObject(nativePythonObject, null));
        }
        PyObject tuple = PyTuple_New(args.length); // leaky; tuple may contain borrowed references, so can not be de-allocated.
        for (int i = 0; i < args.length; i++) {
            PyTuple_SetItem(tuple, i, j2pyObject(args[i]).nativePythonObject);
        }
        PythonObject ret = new PythonObject(PyObject_Call(nativePythonObject, tuple, null));
        return ret;
    }

    public PythonObject callWithArgs(PythonObject args) {
        PyObject tuple = PyList_AsTuple(args.nativePythonObject);
        return new PythonObject(PyObject_Call(nativePythonObject, tuple, null));
    }

    public PythonObject callWithKwargs(PythonObject kwargs) {
        PyObject tuple = PyTuple_New(0);
        return new PythonObject(PyObject_Call(nativePythonObject, tuple, kwargs.nativePythonObject));
    }

    public PythonObject callWithArgsAndKwargs(PythonObject args, PythonObject kwargs) {
        PyObject tuple = PyList_AsTuple(args.nativePythonObject);
        PyObject dict = kwargs.nativePythonObject;
        return new PythonObject(PyObject_Call(nativePythonObject, tuple, dict));
    }

    public PythonObject call(Map kwargs) {
        PyObject dict = new PythonObject(kwargs).nativePythonObject;
        PyObject tuple = PyTuple_New(0);
        return new PythonObject(PyObject_Call(nativePythonObject, tuple, dict));
    }

    public PythonObject call(List args) {
        PyObject tuple = PyList_AsTuple(new PythonObject(args).nativePythonObject);
        return new PythonObject(PyObject_Call(nativePythonObject, tuple, null));
    }

    public PythonObject call(List args, Map kwargs) {
        PyObject tuple = PyList_AsTuple(new PythonObject(args).nativePythonObject);
        PyObject dict = new PythonObject(kwargs).nativePythonObject;
        return new PythonObject(PyObject_Call(nativePythonObject, tuple, dict));
    }

    private PythonObject get(PyObject key) {
        return new PythonObject(
                PyObject_GetItem(nativePythonObject, key)
        );
    }

    public PythonObject get(PythonObject key) {
        return get(key.nativePythonObject);
    }

    public PythonObject get(int key) {
        return get(PyLong_FromLong((long) key));
    }

    public PythonObject get(long key) {
        return new PythonObject(
                PyObject_GetItem(nativePythonObject, PyLong_FromLong(key))
        );
    }

    public PythonObject get(double key) {
        return new PythonObject(
                PyObject_GetItem(nativePythonObject, PyFloat_FromDouble(key))
        );
    }

    public PythonObject get(String key) {
        return get(new PythonObject(key));
    }

    public void set(PythonObject key, PythonObject value) {
        PyObject_SetItem(nativePythonObject, key.nativePythonObject, value.nativePythonObject);
    }

    public void del() {
        Py_DecRef(nativePythonObject);
        nativePythonObject = null;
    }

    public JSONArray toJSONArray() throws PythonException {
        PythonObject json = Python.importModule("json");
        PythonObject serialized = json.attr("dumps").call(this, _getNDArraySerializer());
        String jsonString = serialized.toString();
        return new JSONArray(jsonString);
    }

    public JSONObject toJSONObject() throws PythonException {
        PythonObject json = Python.importModule("json");
        PythonObject serialized = json.attr("dumps").call(this, _getNDArraySerializer());
        String jsonString = serialized.toString();
        return new JSONObject(jsonString);
    }

    public List toList() throws PythonException{
        List list = new ArrayList();
        int n = Python.len(this).toInt();
        for (int i = 0; i < n; i++) {
            PythonObject o = get(i);
            if (Python.isinstance(o, Python.strType())) {
                list.add(o.toString());
            } else if (Python.isinstance(o, Python.intType())) {
                list.add(o.toLong());
            } else if (Python.isinstance(o, Python.floatType())) {
                list.add(o.toDouble());
            } else if (Python.isinstance(o, Python.boolType())) {
                list.add(o);
            } else if (Python.isinstance(o, Python.listType(), Python.tupleType())) {
                list.add(o.toList());
            } else if (Python.isinstance(o, Python.importModule("numpy").attr("ndarray"))) {
                list.add(o.toNumpy().getNd4jArray());
            } else if (Python.isinstance(o, Python.dictType())) {
                list.add(o.toMap());
            } else {
                throw new RuntimeException("Error while converting python" +
                        " list to java List: Unable to serialize python " +
                        "object of type " + Python.type(this).toString());
            }
        }

        return list;
    }

    public Map toMap() throws PythonException{
        Map map = new HashMap();
        List keys = Python.list(attr("keys").call()).toList();
        List values = Python.list(attr("values").call()).toList();
        for (int i = 0; i < keys.size(); i++) {
            map.put(keys.get(i), values.get(i));
        }
        return map;
    }

    public BytePointer toBytePointer() throws PythonException{
        if (Python.isinstance(this, Python.bytesType())){
            PyObject byteArray = PyByteArray_FromObject(nativePythonObject);
            return PyByteArray_AsString(byteArray);

        }
        else if (Python.isinstance(this, Python.bytearrayType())){
            return PyByteArray_AsString(nativePythonObject);
        }
        else if (Python.isinstance(this, Python.memoryviewType())){

//            PyObject np = PyImport_ImportModule("numpy");
//            PyObject array = PyObject_GetAttrString(np, "asarray");
//            PyObject npArr = PyObject_CallObject(array, nativePythonObject); // Doesn't work
            // Invoke interpreter:
            String tempContext = "temp" + UUID.randomUUID().toString().replace('-', '_');
            String originalContext = Python.getCurrentContext();
            Python.setContext(tempContext);
            PythonExecutioner.setVariable("memview", this);
            PythonExecutioner.exec("import numpy as np\narr = np.frombuffer(memview, dtype='int8')");
            INDArray arr = PythonExecutioner.getVariable("arr").toNumpy().getNd4jArray();
            if(arr.data() instanceof BaseDataBuffer){
                ((BaseDataBuffer)arr.data()).syncToPrimary();
            }
            BytePointer ret = new BytePointer(arr.data().pointer());
            Python.setContext(originalContext);
            Python.deleteContext(tempContext);
            return ret;
        } else {
            PyObject ctypes = PyImport_ImportModule("ctypes");
            PyObject cArrType = PyObject_GetAttrString(ctypes, "Array");
            if (PyObject_IsInstance(nativePythonObject, cArrType) != 0){
                PyObject cVoidP = PyObject_GetAttrString(ctypes, "c_void_p");
                PyObject cast = PyObject_GetAttrString(ctypes, "cast");
                PyObject argsTuple = PyTuple_New(2);
                PyTuple_SetItem(argsTuple, 0, nativePythonObject);
                PyTuple_SetItem(argsTuple, 1, cVoidP);
                PyObject voidPtr = PyObject_Call(cast, argsTuple, null);
                PyObject pyAddress = PyObject_GetAttrString(voidPtr, "value");
                long address = PyLong_AsLong(pyAddress);
                long size = PyObject_Size(nativePythonObject);
                Py_DecRef(ctypes);
                Py_DecRef(cArrType);
                Py_DecRef(argsTuple);
                Py_DecRef(voidPtr);
                Py_DecRef(pyAddress);
                Pointer ptr = NativeOpsHolder.getInstance().getDeviceNativeOps().pointerForAddress(address);
                ptr = ptr.limit(size);
                ptr = ptr.capacity(size);
                return new BytePointer(ptr);
            }
            else{
                throw new PythonException("Expected bytes, bytearray, memoryview or ctypesArray. Received " + Python.type(this).toString());
            }
        }
    }
    public boolean isNone() {
       return (nativePythonObject == null)||
               (toString().equals("None") && Python.type(this).toString().equals("<class 'NoneType'>"));
    }
}
