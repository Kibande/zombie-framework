bl_info = {
    "name": "Zombie cfx2 Export",
    "author": "minexew",
    "blender": (2, 71, 0),
    "location": "File > Export",
    "description": "Export a .cfx2 file containing all scene information",
    "warning": "",
    "tracker_url": "",
    "support": 'COMMUNITY',
    "category": "Import-Export"}

if "bpy" in locals():
    import imp
    if "export_cfx2" in locals():
        imp.reload(export_cfx2)

import bpy
from bpy.props import StringProperty, FloatProperty, BoolProperty
from bpy_extras.io_utils import ExportHelper

class Cfx2Export(bpy.types.Operator, ExportHelper):
    """Export selection to a quake map"""
    bl_idname = "export_scene.cfx2"
    bl_label = "Export Zombie cfx2"
    bl_options = {'PRESET'}

    filename_ext = ".cfx2"
    filter_glob = StringProperty(default="*.cfx2", options={'HIDDEN'})

    def execute(self, context):
        # import math
        # from mathutils import Matrix
        if not self.filepath:
            raise Exception("filepath not set")

        keywords = self.as_keywords(ignore=("check_existing", "filter_glob"))

        from . import export_cfx2
        return export_cfx2.save(self, context, **keywords)

def menu_func(self, context):
    self.layout.operator(Cfx2Export.bl_idname, text="cfx2 file (.cfx2)")

def register():
    bpy.utils.register_module(__name__)

    bpy.types.INFO_MT_file_export.append(menu_func)

def unregister():
    bpy.utils.unregister_module(__name__)

    bpy.types.INFO_MT_file_export.remove(menu_func)

if __name__ == "__main__":
    register()
