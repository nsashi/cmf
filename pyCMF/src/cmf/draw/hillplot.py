import numpy
import pylab
import cmf
class hill_plot(object):
    def __get_snow_height(self):
        return [(c.z + c.snow.state/(c.area*0.08) if c.snow else c.z) for c in self.cells]
    def __get_layer_shape(self,layer,c_this,c_left,c_right):
        x_left=0.5*(c_this.x+c_left.x)
        x_right=0.5*(c_this.x+c_right.x)
        x_center=c_this.x
        z_left=0.5*(c_this.z+c_left.z)
        z_center=c_this.z
        z_right=0.5*(c_this.z+c_right.z)
        x=(x_left,x_center,x_right,x_right,x_center,x_left)
        ub,lb=layer.boundary
        z=(z_left-ub,z_center-ub,z_right-ub,z_right-lb,z_center-lb,z_left-lb)
        return x,z
    def __init__(self,cells,t,solute=None):
        was_interactive=pylab.isinteractive()
        if was_interactive: pylab.ioff()
        pylab.clf()
        self.cells=cells
        self.layers=cmf.node_list()
        self.layers.extend(cmf.get_layers(cells))
        self.surface_water=cmf.node_list()
        self.surface_water.extend((c.surface_water for c in cells))
        x_pos=[c.x for c in cells]
        self.topline=pylab.plot(x_pos,self.__get_snow_height() ,'k-',lw=2)[0]
        self.__cells_of_layer={}
        self.polys={}
        if isinstance(solute,cmf.Solute):
            evalfunction= lambda l: l.conc(solute)
        else:
            evalfunction= lambda l: l.wetness
        for i,c in enumerate(cells):
            c_left=cells[i-1] if i else c
            c_right=cells[i+1] if i<len(cells)-1 else c
            
            for l in c.layers:
                x,z=self.__get_layer_shape(l, c, c_left, c_right)
                self.polys[l.node_id],=pylab.fill(x,z,fc=pylab.cm.RdYlBu(evalfunction(l)),ec='0.5',zorder=0)
                self.__cells_of_layer[l.node_id]=(c,c_left,c_right)
        layer_pos=self.layers.get_positions()
        surf_pos=self.surface_water.get_positions()
        w=numpy.array([cmf.AsSoilWater(l).wetness for l in self.layers])
        layer_f=self.layers.get_fluxes3d(t)
        surf_f=self.surface_water.get_fluxes3d(t)
                                         
        self.q_sub=pylab.quiver(layer_pos.X,layer_pos.Z,layer_f.X,layer_f.Z,cmap=pylab.cm.RdYlBu,clim=(0.,1.),scale=25,minlength=0.01,pivot='middle',zorder=1)
        self.q_surf=pylab.quiver(surf_pos.X,surf_pos.Z,surf_f.X,surf_f.Z,color='b',scale=25,minlength=0.01,pivot='middle',zorder=1)
        self.title=pylab.title(t)
        if was_interactive:
            pylab.draw() 
            pylab.ion()
    def __call__(self,t,text='',solute=None):
        layer_f=self.layers.get_fluxes3d(t)
        surf_f=self.surface_water.get_fluxes3d(t)
        if isinstance(solute,cmf.Solute):
            evalfunction= lambda l: l.conc(solute)
        else:
            evalfunction= lambda l: l.wetness
        self.q_sub.set_UVC(numpy.asarray(layer_f.X),numpy.asarray(layer_f.Z))
        self.q_surf.set_UVC(numpy.asarray(surf_f.X),numpy.asarray(surf_f.Z))
        for node in self.layers:
            l=cmf.AsSoilWater(node)
            self.polys[l.node_id].set_fc(pylab.cm.RdYlBu(evalfunction(l)))
            x,z=self.__get_layer_shape(l, *self.__cells_of_layer[l.node_id])
            self.polys[l.node_id].set_xy(numpy.column_stack((x,z)))
        self.topline.set_ydata(self.__get_snow_height())
        if not text: text=str(t)         
        self.title.set_text(text)
        if pylab.isinteractive(): pylab.draw()
    def __set_scale(self,scale):
        self.q_sub.scale=scale
        self.q_surf.scale=scale
    def __get_scale(self):
        return self.q_sub.scale
    scale=property(__get_scale,__set_scale,"The scaling of the arrows")    
   