#include <X11/Xresource.h>
#include <math.h>

int
resource_load(XrmDatabase db, char *name, enum resource_type rtype, void *dst)
{
	char **sdst = dst;
	int *idst = dst;
	float *fdst = dst;

	char fullname[256];
	char *type;
	XrmValue ret;

	snprintf(fullname, sizeof(fullname), "%s.%s", "dack", name);

	fullname[sizeof(fullname) - 1] = '\0';

	XrmGetResource(db, name, fullname, &type, &ret);
	if (ret.addr == NULL || strncmp("String", type, 64))
		return 1;

	switch (rtype) {
	case STRING:
		free(*sdst);
		*sdst = strdup(ret.addr);
		break;
	case INTEGER:
		*idst = strtoul(ret.addr, NULL, 10);
		break;
	case FLOAT:
		*fdst = strtof(ret.addr, NULL);
		break;
	}
	return 0;
}

void
xrdb_init(Display *dpy)
{
	char *resm;
	XrmDatabase db;
	int i;
	ResourcePref *p;
	XrmInitialize();
	resm = XResourceManagerString(dpy);
	if (!resm)
		return;

	db = XrmGetStringDatabase(resm);
	for (i = 0; i < num_colors; i++) {
		p = colors[i].resource;
		if (!p)
			continue;
		resource_load(db, p->name, p->type, p->dst);
	}
}
