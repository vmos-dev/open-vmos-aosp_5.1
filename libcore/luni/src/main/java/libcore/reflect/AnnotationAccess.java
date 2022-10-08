/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package libcore.reflect;

import com.android.dex.Dex;
import com.android.dex.EncodedValueReader;
import com.android.dex.FieldId;
import com.android.dex.MethodId;
import com.android.dex.ProtoId;
import com.android.dex.TypeList;
import java.lang.annotation.Annotation;
import java.lang.annotation.Inherited;
import java.lang.reflect.AccessibleObject;
import java.lang.reflect.AnnotatedElement;
import java.lang.reflect.Array;
import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.Member;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import libcore.util.EmptyArray;

/**
 * Look up annotations from a dex file.
 */
public final class AnnotationAccess {
    private AnnotationAccess() {
    }

    /*
     * Classes like arrays, primitives and proxies will not have a Dex file.
     * Such classes never have annotations.
     */

    private static final Class<?>[] NO_ARGUMENTS = null;
    @SuppressWarnings("unused")
    private static final byte VISIBILITY_BUILD = 0x00;
    private static final byte VISIBILITY_RUNTIME = 0x01;
    @SuppressWarnings("unused")
    private static final byte VISIBILITY_SYSTEM = 0x02;

    /*
     * Class annotations. This includes declared class annotations plus
     * annotations on the superclass that have @Inherited.
     */

    public static <A extends java.lang.annotation.Annotation> A getAnnotation(
            Class<?> c, Class<A> annotationType) {
        if (annotationType == null) {
            throw new NullPointerException("annotationType == null");
        }

        A annotation = getDeclaredAnnotation(c, annotationType);
        if (annotation != null) {
            return annotation;
        }

        if (isInherited(annotationType)) {
            for (Class<?> sup = c.getSuperclass(); sup != null; sup = sup.getSuperclass()) {
                annotation = getDeclaredAnnotation(sup, annotationType);
                if (annotation != null) {
                    return annotation;
                }
            }
        }

        return null;
    }

    /**
     * Returns true if {@code annotationType} annotations on the superclass
     * apply to subclasses that don't have another annotation of the same
     * type.
     */
    private static boolean isInherited(Class<? extends Annotation> annotationType) {
        return isDeclaredAnnotationPresent(annotationType, Inherited.class);
    }

    public static Annotation[] getAnnotations(Class<?> c) {
        /*
         * We need to get the annotations declared on this class, plus the
         * annotations from superclasses that have the "@Inherited" annotation
         * set.  We create a temporary map to use while we accumulate the
         * annotations and convert it to an array at the end.
         *
         * It's possible to have duplicates when annotations are inherited.
         * We use a Map to filter those out.
         *
         * HashMap might be overkill here.
         */
        HashMap<Class<?>, Annotation> map = new HashMap<Class<?>, Annotation>();
        for (Annotation declaredAnnotation : getDeclaredAnnotations(c)) {
            map.put(declaredAnnotation.annotationType(), declaredAnnotation);
        }
        for (Class<?> sup = c.getSuperclass(); sup != null; sup = sup.getSuperclass()) {
            for (Annotation declaredAnnotation : getDeclaredAnnotations(sup)) {
                Class<? extends Annotation> clazz = declaredAnnotation.annotationType();
                if (!map.containsKey(clazz) && isInherited(clazz)) {
                    map.put(clazz, declaredAnnotation);
                }
            }
        }

        /* convert annotation values from HashMap to array */
        Collection<Annotation> coll = map.values();
        return coll.toArray(new Annotation[coll.size()]);
    }

    /**
     * Returns true if {@code c} is annotated by {@code annotationType}.
     */
    public static boolean isAnnotationPresent(
            Class<?> c, Class<? extends Annotation> annotationType) {
        if (annotationType == null) {
            throw new NullPointerException("annotationType == null");
        }

        if (isDeclaredAnnotationPresent(c, annotationType)) {
            return true;
        }

        if (isInherited(annotationType)) {
            for (Class<?> sup = c.getSuperclass(); sup != null; sup = sup.getSuperclass()) {
                if (isDeclaredAnnotationPresent(sup, annotationType)) {
                    return true;
                }
            }
        }

        return false;
    }

    /*
     * Class, Field, Method, Constructor and Parameter annotations
     */

    /**
     * Returns the annotations on {@code element}.
     */
    public static List<Annotation> getDeclaredAnnotations(AnnotatedElement element) {
        int offset = getAnnotationSetOffset(element);
        return annotationSetToAnnotations(getDexClass(element), offset);
    }

    /**
     * Returns the annotation if it exists.
     */
    public static <A extends Annotation> A getDeclaredAnnotation(
            AnnotatedElement element, Class<A> annotationClass) {
        com.android.dex.Annotation a = getAnnotation(element, annotationClass);
        return a != null
                ? toAnnotationInstance(getDexClass(element), annotationClass, a)
                : null;
    }

    /**
     * Returns true if the annotation exists.
     */
    public static boolean isDeclaredAnnotationPresent(
            AnnotatedElement element, Class<? extends Annotation> annotationClass) {
        return getAnnotation(element, annotationClass) != null;
    }

    private static com.android.dex.Annotation getAnnotation(
            AnnotatedElement element, Class<? extends Annotation> annotationClass) {
        int annotationSetOffset = getAnnotationSetOffset(element);
        if (annotationSetOffset == 0) {
            return null; // no annotation
        }

        Class<?> dexClass = getDexClass(element);
        Dex dex = dexClass.getDex();
        Dex.Section setIn = dex.open(annotationSetOffset); // annotation_set_item
        String annotationInternalName = InternalNames.getInternalName(annotationClass);
        for (int i = 0, size = setIn.readInt(); i < size; i++) {
            int annotationOffset = setIn.readInt();
            Dex.Section annotationIn = dex.open(annotationOffset); // annotation_item
            // The internal string name of the annotation is compared here and deliberately not
            // the value of annotationClass.getTypeIndex(). The annotationClass may have been
            // defined by a different dex file, which would make the indexes incomparable.
            com.android.dex.Annotation candidate = annotationIn.readAnnotation();
            String candidateInternalName = dex.typeNames().get(candidate.getTypeIndex());
            if (candidateInternalName.equals(annotationInternalName)) {
                return candidate;
            }
        }

        return null; // This set doesn't contain the annotation.
    }

    /**
     * @param element a class, a field, a method or a constructor.
     */
    private static int getAnnotationSetOffset(AnnotatedElement element) {
        Class<?> dexClass = getDexClass(element);
        int directoryOffset = dexClass.getDexAnnotationDirectoryOffset();
        if (directoryOffset == 0) {
            return 0; // nothing on this class has annotations
        }

        Dex.Section directoryIn = dexClass.getDex().open(directoryOffset);
        int classSetOffset = directoryIn.readInt();
        if (element instanceof Class) {
            return classSetOffset;
        }

        int fieldsSize = directoryIn.readInt();
        int methodsSize = directoryIn.readInt();
        directoryIn.readInt(); // parameters size

        if (element instanceof Field) {
            int fieldIndex = ((Field) element).getDexFieldIndex();
            for (int i = 0; i < fieldsSize; i++) {
                int candidateFieldIndex = directoryIn.readInt();
                int annotationSetOffset = directoryIn.readInt();
                if (candidateFieldIndex == fieldIndex) {
                    return annotationSetOffset;
                }
            }
            // if we were searching for a field then we missed
            return 0;
        }

        // Skip through the fields without reading them and look for constructors or methods.
        directoryIn.skip(8 * fieldsSize);

        int methodIndex= element instanceof Method ? ((Method) element).getDexMethodIndex()
                                                   : ((Constructor<?>) element).getDexMethodIndex();
        for (int i = 0; i < methodsSize; i++) {
            int candidateMethodIndex = directoryIn.readInt();
            int annotationSetOffset = directoryIn.readInt();
            if (candidateMethodIndex == methodIndex) {
                return annotationSetOffset;
            }
        }

        return 0;
    }

    /**
     * Returns {@code element} if it is a class; and the class declaring
     * {@code element} otherwise. The dex file of the returned class also
     * defines {@code element}.
     */
    private static Class<?> getDexClass(AnnotatedElement element) {
        return element instanceof Class
                ? ((Class<?>) element)
                : ((Member) element).getDeclaringClass();
    }

    /**
     * Returns the parameter annotations on {@code member}.
     */
    public static Annotation[][] getParameterAnnotations(Class<?> declaringClass,
                                                         int methodDexIndex) {
        Dex dex = declaringClass.getDex();
        int protoIndex = dex.methodIds().get(methodDexIndex).getProtoIndex();
        ProtoId proto = dex.protoIds().get(protoIndex);
        TypeList parametersList = dex.readTypeList(proto.getParametersOffset());
        short[] types = parametersList.getTypes();
        int typesCount = types.length;

        int directoryOffset = declaringClass.getDexAnnotationDirectoryOffset();
        if (directoryOffset == 0) {
            return new Annotation[typesCount][0]; // nothing on this class has annotations
        }

        Dex.Section directoryIn = dex.open(directoryOffset);
        directoryIn.readInt(); // class annotations
        int fieldsSize = directoryIn.readInt();
        int methodsSize = directoryIn.readInt();
        int parametersSize = directoryIn.readInt();

        for (int i = 0; i < fieldsSize; i++) {
            directoryIn.readInt(); // field_index
            directoryIn.readInt(); // annotation_set
        }

        for (int i = 0; i < methodsSize; i++) {
            directoryIn.readInt(); // method_index
            directoryIn.readInt(); // annotation_set
        }

        for (int i = 0; i < parametersSize; i++) {
            int candidateMethodDexIndex = directoryIn.readInt();
            int annotationSetRefListOffset = directoryIn.readInt();
            if (candidateMethodDexIndex != methodDexIndex) {
                continue;
            }

            Dex.Section refList = dex.open(annotationSetRefListOffset);
            int parameterCount = refList.readInt();
            Annotation[][] result = new Annotation[parameterCount][];
            for (int p = 0; p < parameterCount; p++) {
                int annotationSetOffset = refList.readInt();
                List<Annotation> annotations
                        = annotationSetToAnnotations(declaringClass, annotationSetOffset);
                result[p] = annotations.toArray(new Annotation[annotations.size()]);
            }
            return result;
        }

        return new Annotation[typesCount][0];
    }

    /*
     * System annotations.
     */

    public static Object getDefaultValue(Method method) {
        /*
         * Dex represents this with @AnnotationDefault on annotations that have
         * default values:
         *
         * @AnnotationDefault(value=@Foo(a=7))
         * public @interface Foo {
         *   int a() default 7;
         *   int b();
         * }
         */

        Class<?> annotationClass = method.getDeclaringClass();
        // All lookups of type and string indexes are within the Dex that declares the annotation so
        // the indexes can be compared directly.
        Dex dex = annotationClass.getDex();
        EncodedValueReader reader = getOnlyAnnotationValue(
                dex, annotationClass, "Ldalvik/annotation/AnnotationDefault;");
        if (reader == null) {
            return null;
        }

        int fieldCount = reader.readAnnotation();
        if (reader.getAnnotationType() != annotationClass.getDexTypeIndex()) {
            throw new AssertionError("annotation value type != annotation class");
        }

        int methodNameIndex = dex.findStringIndex(method.getName());
        for (int i = 0; i < fieldCount; i++) {
            int candidateNameIndex = reader.readAnnotationName();
            if (candidateNameIndex == methodNameIndex) {
                Class<?> returnType = method.getReturnType();
                return decodeValue(annotationClass, returnType, dex, reader);
            } else {
                reader.skipValue();
            }
        }

        return null;
    }

    /**
     * Returns the class of which {@code c} is a direct member. If {@code c} is
     * defined in a method or constructor, this is not transitive.
     */
    public static Class<?> getEnclosingClass(Class<?> c) {
        /*
         * public class Bar {
         *   @EnclosingClass(value=Bar)
         *   public class Foo {}
         * }
         */
        Dex dex = c.getDex();
        EncodedValueReader reader = getOnlyAnnotationValue(
                dex, c, "Ldalvik/annotation/EnclosingClass;");
        if (reader == null) {
            return null;
        }
        return c.getDexCacheType(dex, reader.readType());
    }

    public static AccessibleObject getEnclosingMethodOrConstructor(Class<?> c) {
        /*
         * public class Bar {
         *   public void quux(String s, int i) {
         *     @EnclosingMethod(value=Bar.quux(String,int))
         *     class Foo {}
         *   }
         * }
         */
        Dex dex = c.getDex();
        EncodedValueReader reader = getOnlyAnnotationValue(
                dex, c, "Ldalvik/annotation/EnclosingMethod;");
        if (reader == null) {
            return null;
        }
        return indexToMethod(c, dex, reader.readMethod());
    }

    public static Class<?>[] getMemberClasses(Class<?> c) {
        /*
         * @MemberClasses(value=[Bar, Baz])
         * public class Foo {
         *   class Bar {}
         *   class Baz {}
         * }
         */
        Dex dex = c.getDex();
        EncodedValueReader reader = getOnlyAnnotationValue(
                dex, c, "Ldalvik/annotation/MemberClasses;");
        if (reader == null) {
            return EmptyArray.CLASS;
        }
        return (Class[]) decodeValue(c, Class[].class, dex, reader);
    }

    /**
     * @param element a class, a field, a method or a constructor.
     */
    public static String getSignature(AnnotatedElement element) {
        /*
         * @Signature(value=["Ljava/util/List", "<", "Ljava/lang/String;", ">;"])
         * List<String> foo;
         */
        Class<?> dexClass = getDexClass(element);
        Dex dex = dexClass.getDex();
        EncodedValueReader reader = getOnlyAnnotationValue(
                dex, element, "Ldalvik/annotation/Signature;");
        if (reader == null) {
            return null;
        }
        String[] array = (String[]) decodeValue(dexClass, String[].class, dex, reader);
        StringBuilder result = new StringBuilder();
        for (String s : array) {
            result.append(s);
        }
        return result.toString();
    }

    /**
     * @param element a method or a constructor.
     */
    public static Class<?>[] getExceptions(AnnotatedElement element) {
        /*
         * @Throws(value=[IOException.class])
         * void foo() throws IOException;
         */
        Class<?> dexClass = getDexClass(element);
        Dex dex = dexClass.getDex();
        EncodedValueReader reader = getOnlyAnnotationValue(
                dex, element, "Ldalvik/annotation/Throws;");
        if (reader == null) {
            return EmptyArray.CLASS;
        }
        return (Class<?>[]) decodeValue(dexClass, Class[].class, dex, reader);
    }

    public static int getInnerClassFlags(Class<?> c, int defaultValue) {
        /*
         * @InnerClass(accessFlags=0x01,name="Foo")
         * class Foo {};
         */
        Dex dex = c.getDex();
        EncodedValueReader reader = getAnnotationReader(
                dex, c, "Ldalvik/annotation/InnerClass;", 2);
        if (reader == null) {
            return defaultValue;
        }
        reader.readAnnotationName(); // accessFlags
        return reader.readInt();
    }

    public static String getInnerClassName(Class<?> c) {
        /*
         * @InnerClass(accessFlags=0x01,name="Foo")
         * class Foo {};
         */
        Dex dex = c.getDex();
        EncodedValueReader reader = getAnnotationReader(
                dex, c, "Ldalvik/annotation/InnerClass;", 2);
        if (reader == null) {
            return null;
        }
        reader.readAnnotationName(); // accessFlags
        reader.readInt();
        reader.readAnnotationName(); // name
        return reader.peek() == EncodedValueReader.ENCODED_NULL
                ? null
                : (String) decodeValue(c, String.class, dex, reader);
    }

    public static boolean isAnonymousClass(Class<?> c) {
        /*
         * @InnerClass(accessFlags=0x01,name="Foo")
         * class Foo {};
         */
        Dex dex = c.getDex();
        EncodedValueReader reader = getAnnotationReader(
                dex, c, "Ldalvik/annotation/InnerClass;", 2);
        if (reader == null) {
            return false;
        }
        reader.readAnnotationName(); // accessFlags
        reader.readInt();
        reader.readAnnotationName(); // name
        return reader.peek() == EncodedValueReader.ENCODED_NULL;
    }

    /*
     * Dex support.
     *
     * Different classes come from different Dex files. This class is careful
     * to guarantee that Dex-relative indices and encoded values are interpreted
     * using the Dex that they were read from. Methods that use Dex-relative
     * values accept that Dex as a parameter or the class from which that Dex
     * was derived.
     */

    private static EncodedValueReader getAnnotationReader(
            Dex dex, AnnotatedElement element, String annotationName, int expectedFieldCount) {
        int annotationSetOffset = getAnnotationSetOffset(element);
        if (annotationSetOffset == 0) {
            return null; // no annotations on the class
        }

        Dex.Section setIn = dex.open(annotationSetOffset); // annotation_set_item
        com.android.dex.Annotation annotation = null;
        // TODO: is it better to compute the index of the annotation name in the dex file and check
        //       indices below?
        for (int i = 0, size = setIn.readInt(); i < size; i++) {
            int annotationOffset = setIn.readInt();
            Dex.Section annotationIn = dex.open(annotationOffset); // annotation_item
            com.android.dex.Annotation candidate = annotationIn.readAnnotation();
            String candidateAnnotationName = dex.typeNames().get(candidate.getTypeIndex());
            if (annotationName.equals(candidateAnnotationName)) {
                annotation = candidate;
                break;
            }
        }
        if (annotation == null) {
            return null; // no annotation
        }

        EncodedValueReader reader = annotation.getReader();
        int fieldCount = reader.readAnnotation();
        String readerAnnotationName = dex.typeNames().get(reader.getAnnotationType());
        if (!readerAnnotationName.equals(annotationName)) {
            throw new AssertionError();
        }
        if (fieldCount != expectedFieldCount) {
            return null; // not the expected values on this annotation; give up
        }

        return reader;
    }

    /**
     * Returns a reader ready to read the only value of the annotation on
     * {@code element}, or null if that annotation doesn't exist.
     */
    private static EncodedValueReader getOnlyAnnotationValue(
            Dex dex, AnnotatedElement element, String annotationName) {
        EncodedValueReader reader = getAnnotationReader(dex, element, annotationName, 1);
        if (reader == null) {
            return null;
        }
        reader.readAnnotationName(); // skip the name
        return reader;
    }

    private static Class<? extends Annotation> getAnnotationClass(Class<?> context, Dex dex,
                                                                  int typeIndex) {
        try {
            @SuppressWarnings("unchecked") // we do a runtime check
            Class<? extends Annotation> result =
                (Class<? extends Annotation>) context.getDexCacheType(dex, typeIndex);
            if (!result.isAnnotation()) {
                throw new IncompatibleClassChangeError("Expected annotation: " + result.getName());
            }
            return result;
        } catch (NoClassDefFoundError ncdfe) {
            return null;
        }
    }

    private static AccessibleObject indexToMethod(Class<?> context, Dex dex, int methodIndex) {
        Class<?> declaringClass =
            context.getDexCacheType(dex, dex.declaringClassIndexFromMethodIndex(methodIndex));
        String name = context.getDexCacheString(dex, dex.nameIndexFromMethodIndex(methodIndex));
        short[] types = dex.parameterTypeIndicesFromMethodIndex(methodIndex);
        Class<?>[] parametersArray = new Class[types.length];
        for (int i = 0; i < types.length; i++) {
            parametersArray[i] = context.getDexCacheType(dex, types[i]);
        }
        try {
            return name.equals("<init>")
                ? declaringClass.getDeclaredConstructor(parametersArray)
                : declaringClass.getDeclaredMethod(name, parametersArray);
        } catch (NoSuchMethodException e) {
            throw new IncompatibleClassChangeError("Couldn't find " + declaringClass.getName()
                                                   + "." + name + Arrays.toString(parametersArray));
        }
    }

    private static List<Annotation> annotationSetToAnnotations(Class<?> context, int offset) {
        if (offset == 0) {
            return Collections.emptyList(); // no annotations in the set
        }

        Dex dex = context.getDex();
        Dex.Section setIn = dex.open(offset); // annotation_set_item
        int size = setIn.readInt();
        List<Annotation> result = new ArrayList<Annotation>(size);

        for (int i = 0; i < size; i++) {
            int annotationOffset = setIn.readInt();
            Dex.Section annotationIn = dex.open(annotationOffset); // annotation_item
            com.android.dex.Annotation annotation = annotationIn.readAnnotation();
            if (annotation.getVisibility() != VISIBILITY_RUNTIME) {
                continue;
            }
            Class<? extends Annotation> annotationClass =
                    getAnnotationClass(context, dex, annotation.getTypeIndex());
            if (annotationClass != null) {
                result.add(toAnnotationInstance(context, dex, annotationClass, annotation.getReader()));
            }
        }
        return result;
    }

    private static <A extends Annotation> A toAnnotationInstance(Class<?> context,
            Class<A> annotationClass, com.android.dex.Annotation annotation) {
        return toAnnotationInstance(context, context.getDex(), annotationClass,
                annotation.getReader());
    }

    private static <A extends Annotation> A toAnnotationInstance(Class<?> context, Dex dex,
            Class<A> annotationClass, EncodedValueReader reader) {
        int fieldCount = reader.readAnnotation();
        if (annotationClass != context.getDexCacheType(dex, reader.getAnnotationType())) {
            throw new AssertionError("annotation value type != return type");
        }
        AnnotationMember[] members = new AnnotationMember[fieldCount];
        for (int i = 0; i < fieldCount; i++) {
            int name = reader.readAnnotationName();
            String nameString = dex.strings().get(name);
            Method method;
            try {
                method = annotationClass.getMethod(nameString, NO_ARGUMENTS);
            } catch (NoSuchMethodException e) {
                throw new IncompatibleClassChangeError(
                        "Couldn't find " + annotationClass.getName() + "." + nameString);
            }
            Class<?> returnType = method.getReturnType();
            Object value = decodeValue(context, returnType, dex, reader);
            members[i] = new AnnotationMember(nameString, value, returnType, method);
        }
        return AnnotationFactory.createAnnotation(annotationClass, members);
    }

    private static Object decodeValue(Class<?> context, Class<?> type,
            Dex dex, EncodedValueReader reader) {
        if (type.isArray()) {
            int size = reader.readArray();
            Class<?> componentType = type.getComponentType();
            Object array = Array.newInstance(componentType, size);
            for (int i = 0; i < size; i++) {
                Array.set(array, i, decodeValue(context, componentType, dex, reader));
            }
            return array;
        } else if (type.isEnum()) {
            int fieldIndex = reader.readEnum();
            FieldId fieldId = dex.fieldIds().get(fieldIndex);
            String fieldName = dex.strings().get(fieldId.getNameIndex());
            Field field;
            try {
                field = type.getDeclaredField(fieldName);
                return field.get(null);
            } catch (NoSuchFieldException e) {
                NoSuchFieldError error = new NoSuchFieldError();
                error.initCause(e);
                throw error;
            } catch (IllegalAccessException e) {
                IllegalAccessError error = new IllegalAccessError();
                error.initCause(e);
                throw error;
            }
        } else if (type.isAnnotation()) {
            @SuppressWarnings("unchecked") // Class.isAnnotation is the runtime check
            Class<? extends Annotation> annotationClass = (Class<? extends Annotation>) type;
            return toAnnotationInstance(context, dex, annotationClass, reader);
        } else if (type == String.class) {
            int index = reader.readString();
            return context.getDexCacheString(dex, index);
        } else if (type == Class.class) {
            int index = reader.readType();
            return context.getDexCacheType(dex, index);
        } else if (type == byte.class) {
            return reader.readByte();
        } else if (type == short.class) {
            return reader.readShort();
        } else if (type == int.class) {
            return reader.readInt();
        } else if (type == long.class) {
            return reader.readLong();
        } else if (type == float.class) {
            return reader.readFloat();
        } else if (type == double.class) {
            return reader.readDouble();
        } else if (type == char.class) {
            return reader.readChar();
        } else if (type == boolean.class) {
            return reader.readBoolean();
        } else {
            // is null legit?
            throw new AssertionError("Unexpected annotation value type: " + type);
        }
    }
}
