// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.jnigenerator.annotations;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

/**
 * The annotated method or class verifies on S, but not below.
 *
 * The annotated method (or methods on the annotated class) are guaranteed to not be inlined by R8
 * on builds targeted below S. This prevents class verification errors (which results in a very slow
 * retry-verification-at-runtime) from spreading into other classes on these lower versions.
 */
@Target({ElementType.CONSTRUCTOR, ElementType.METHOD, ElementType.TYPE})
@Retention(RetentionPolicy.CLASS)
public @interface VerifiesOnS {}