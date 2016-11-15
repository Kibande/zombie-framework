import bpy, os

def escape(value):
    return "'%s'" % value.replace("\\", "\\\\").replace("'", "\\'")

def indented(f, indent, *args):
    print('    ' * indent, end='', file=f)
    print(*args, file=f)

def dump_object(f, obj, indent, scene, transform=None):
    if obj.type == 'EMPTY':
        if obj.is_duplicator:
            for linked_obj in obj.dupli_group.objects:
                dump_object(f, linked_obj, indent, scene, obj.matrix_world)
        return

    indented(f, indent, "%s: %s {type=%s mode=%s}" % (type(obj).__name__, escape(obj.name), obj.type, obj.mode))
    indent += 1

    if obj.type == 'MESH':
        dump_mesh(f, obj, transform, indent, scene)
    else:
        indented(f, indent, 'location: %s' % escape(' '.join(map(str, obj.location))))

        dump_value(f, 'data', obj.data, indent, scene)
        dump_value(f, 'matrix_world', obj.matrix_world, indent, scene)
        dump_value(f, 'type', obj.type, indent, scene)

def has_attribute(obj, name):
    return name in dir(obj)

def dump_mesh(f, obj, transform, indent, scene):
    mesh = obj.to_mesh(scene, True, 'PREVIEW')

    # transform to world space
    mesh.transform(obj.matrix_world)

    if transform != None:
        mesh.transform(transform)

    mesh.update(calc_tessface=True)

    dump_value(f, 'mesh', mesh, indent, scene)
    dump_value(f, 'type', obj.type, indent, scene)

def dump_value(f, name, value, indent, scene):
    verbose = False

    if type(value).__name__ == 'Matrix':
        indented(f, indent, '%s: %s' % (name, escape(' '.join(map(lambda value: ' '.join(map(str, value)), value)))))
        return
    elif type(value).__name__ == 'MeshTessFace':
        indented(f, indent, "%s" % type(value).__name__)
        indented(f, indent + 1, "index: %s" % escape(str(value.index)))
        indented(f, indent + 1, "material_index: %s" % escape(str(value.material_index)))
        indented(f, indent + 1, "vertices: %s" % escape(' '.join(map(str, value.vertices))))
        return
    elif type(value).__name__ == 'MeshVertex':
        indented(f, indent, "%s" % type(value).__name__)
        indented(f, indent + 1, "co: %s" % escape(' '.join(map(str, value.co))))
        indented(f, indent + 1, "normal: %s" % escape(' '.join(map(str, value.normal))))
        return
    elif type(value).__name__ == 'Object' and has_attribute(value, 'type') and value.type in ['EMPTY', 'LAMP', 'MESH']:
        if value.hide:
            indented(f, indent, "{ skipping hidden Object %s }" % value.name)
            return
        else:
            return dump_object(f, value, indent, scene)

    if type(value).__name__ == 'Color':
        indented(f, indent, "%s: %s { %s }" % (escape(name), escape(' '.join(map(str, value))),
            type(value).__name__))
    else:
        indented(f, indent, "%s: %s { %s }" % (escape(name), escape(repr(value) if not isinstance(value, str) else value),
            type(value).__name__))

    if indent == 10:
        return

    if (type(value).__name__ == 'Color' or type(value).__name__ == 'Matrix' or type(value).__name__ == 'Vector'
            or isinstance(value, float) or isinstance(value, int) or isinstance(value, range)
            or isinstance(value, str) or isinstance(value, tuple)):
        return

    if type(value).__name__ == 'bpy_prop_array' or type(value).__name__ == 'bpy_prop_collection':
        if name == 'split_normals':
            for item in value:
                indented(f, indent + 1, "'split_normals[]': '%g, %g, %g'" % tuple(item))
        else:
            for item in value:
                dump_value(f, '_', item, indent + 1, scene)

        #indented(f, indent + 1, "{ end of '%s' }" % name)
    else:
        quiet = not verbose

        if name == 'scene':
            want = ['objects', 'world']
        elif name == 'world':
            want = ['ambient_color']
        elif type(value).__name__ == 'Image':
            want = 'filepath name'.split(' ')
            quiet = True
        elif type(value).__name__ == 'ImageTexture':
            want = 'image type'.split(' ')
        elif type(value).__name__ == 'Material':
            want = 'active_texture'.split(' ')
        elif type(value).__name__ == 'Mesh':
            #want = 'edges loops materials polygons vertices'.split(' ')
            want = 'materials tessface_uv_textures tessfaces vertices'.split(' ')
        elif type(value).__name__ == 'MeshEdge':
            want = 'index key'.split(' ')
            quiet = True
        elif type(value).__name__ == 'MeshLoop':
            want = 'edge_index index normal vertex_index'.split(' ')
            quiet = True
        elif type(value).__name__ == 'MeshPolygon':
            want = 'index loop_indices material_index normal vertices'.split(' ')
            quiet = True
        elif type(value).__name__ == 'MeshTextureFace':
            want = 'image uv'.split(' ')
            quiet = True
        elif type(value).__name__ == 'MeshVertex':
            want = 'co index normal undeformed_co'.split(' ')
            quiet = True
        #elif type(value).__name__ == 'Object':
        #    want = 'location name type'.split(' ')
        elif type(value).__name__ == 'PointLamp':
            want = 'color distance energy falloff_type linear_attenuation quadratic_attenuation type'.split(' ')
        elif type(value).__name__ == 'ShaderNodeTree':
            want = []
        elif type(value).__name__ == 'SpotLamp':
            want = ('color distance energy falloff_type halo_intensity halo_step linear_attenuation' +
                    'quadratic_attenuation spot_blend spot_size type').split(' ')
        elif type(value).__name__ == 'Texture':
            want = 'type'.split(' ')
        else:
            want = None

        for field in dir(value):
            if field.startswith('_'):
                continue

            if want is None or field in want:
                print('dump %s in %s' % (field, type(value).__name__))
                if field == 'bl_rna' or field == 'center' or field == 'edge_keys' or field == 'rna_type':
                    indented(f, indent + 1, "{ skipping buggy '%s' }" % field)
                else:
                    dump_value(f, field, getattr(value, field), indent + 1, scene)
            elif not quiet:
                indented(f, indent + 1, "{ ignoring '%s' }" % field)

def export_cfx2(context, filepath):
    import time
    from mathutils import Matrix
    from bpy_extras import mesh_utils

    t = time.time()
    f = open(filepath, 'w')

    scene = context.scene
    objects = context.selected_objects

    dump_value(f, 'scene', scene, 0, scene)

    f.close()

    #print("Exported Map in %.4fsec" % (time.time() - t))
    #print("Brushes: %d  Nodes: %d  Lamps %d\n" % (TOTBRUSH, TOTNODE, TOTLAMP))

def save(operator,
         context,
         filepath=None):

    export_cfx2(context, filepath)

    return {'FINISHED'}
