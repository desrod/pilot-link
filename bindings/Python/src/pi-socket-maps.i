// -*- C -*-

//
// pi-sockaddr... the real structure might be defined in one of two
// different ways, but luckily SWIG doesn't really care.
//
%typemap (python,in) struct sockaddr *INPUT {
    static struct pi_sockaddr temp;
    char *dev;
    int len;

    if (!PyArg_ParseTuple($input, "is#", &temp.pi_family, &dev, &len)) {
	return NULL;
    }
    if (len > 255) {
      // Should really raise an exception
      len = 255;
    }
    strncpy(temp.pi_device, dev, len);
    temp.pi_device[len] = 0;

    $1 = (struct sockaddr *)&temp;
}

%typemap (python, argout) struct sockaddr *OUTPUT {
    PyObject *o;

    if ($1) {
	o = Py_BuildValue("(is)", (int)((struct pi_sockaddr *)$1)->pi_family,
			  ((struct pi_sockaddr *)$1)->pi_device);
	$result = t_output_helper($result, o);
    }
}

%typemap (python,in,numinputs=0) struct sockaddr *OUTPUT (struct pi_sockaddr temp) {
    $1 = (struct sockaddr *)&temp;
}

%typemap (python,in,numinputs=0) int addrlen {
    $1 = sizeof(struct pi_sockaddr);
}
