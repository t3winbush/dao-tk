#django imports
from django.shortcuts import render
from django.http import HttpResponse, Http404
# Other package imports
from datetime import datetime

# Create your views here.
#-------------------------------------------------------------
#-------------------------------------------------------------
def index(request, path):
    
    context = {
        'connection_status' : True,
        'model_status' : False,
        'last_refresh' : datetime.now(),
    }

    #handle URL based on path
    pvals = path.split('/')

    if 'configure' in pvals[0]:
        if len(pvals) != 2:
            return Http404(request)
        
        if 'event_log' in pvals[1]:
            return configure_event_log(request, context)
        elif 'logging' in pvals[1]:
            return configure_logging(request, context)
        elif 'model_settings' in pvals[1]:
            return configure_model_settings(request, context)
        elif 'sources' in pvals[1]:
            return configure_sources(request, context)
        elif 'ui_settings' in pvals[1]:
            return configure_ui_settings(request, context)
        else:
            return Http404(request)

    elif 'whatif' in pvals[0]:
        if len(pvals) != 2:
            return Http404(request)
        
        if 'interruptions' in pvals[1]:
            return whatif_interruptions(request, context)
        elif 'resources' in pvals[1]:
            return whatif_resources(request, context)
        elif 'weather' in pvals[1]:
            return whatif_weather(request, context)
        else:
            return Http404(request)
    else:
        if 'dashboard' in pvals[0]:
            return dashboard(request, context)
        elif 'outlook' in pvals[0]:
            return outlook(request, context)
        elif 'planning' in pvals[0]:
            return planning(request, context)
        elif 'system' in pvals[0]:
            return system(request, context)
        else:
            return Http404(request)

    return 

#-------------------------------------------------------------
def dashboard(request, context={}):

    bar_width = 160
    #get cumulative production for today
    production_daily_total_model = 110*14*0.75
    production_daily_actual = production_daily_total_model*0.4
    production_daily_model = production_daily_actual*1.2
    
    context["progress"] = {
        "progress" : [150/production_daily_total_model * bar_width, 150],
        "lag" : [0/production_daily_total_model * bar_width, 0],
        "lead" : [18/production_daily_total_model * bar_width, 18],
        "remain" : [70/production_daily_total_model*bar_width, 70],
        "total" : [1, production_daily_total_model],
    }

    #----get current production levels----
    #gross power
    gross_actual = 122
    gross_model = 108.5
    gross_difference = abs(gross_actual - gross_model)
    gross_barscale = gross_actual + gross_difference
    #net power
    net_actual = 113.5
    net_model = 102.2
    net_difference = abs(net_actual - net_model)
    net_barscale = net_actual + net_difference
    #storage charge state
    storage_is_charging = True
    storage_max = 8*120/.41
    storage_actual = storage_max * 0.6
    storage_model = storage_actual * 0.7
    storage_difference = storage_actual - storage_model
    storage_exceeds_model = bool(storage_actual > storage_model) if storage_is_charging else bool(storage_model > storage_actual)
    #revenue
    revenue_actual = 122
    revenue_model = 108.5
    revenue_difference = abs(revenue_actual - revenue_model)
    revenue_barscale = revenue_actual + revenue_difference

    context["production"] = {
        "gross":{
            "actual":[gross_actual/gross_barscale*bar_width, gross_actual], 
            "model":[gross_difference/gross_barscale*bar_width, gross_model],
            "exceeds_model":bool(gross_actual > gross_model),
            "difference": abs(gross_actual - gross_model)
            }, 
        "net":{
            "actual":[net_actual/net_barscale*bar_width, net_actual], 
            "difference":[net_difference/net_barscale*bar_width, net_difference],
            "model":[net_model/net_barscale*bar_width, net_model],
            "exceeds_model":bool(net_actual > net_model),
            }, 
        "storage":{
            "primary_bar":min([storage_actual, storage_model])/storage_max*bar_width,
            "difference":[abs(storage_difference)/storage_max*bar_width, storage_difference],
            "exceeds_model":storage_exceeds_model,
            "actual":storage_actual,
            "model":storage_model,
            "balance":(storage_max - min([storage_actual, storage_model]) - abs(storage_difference))/storage_max*bar_width,
        },
        "revenue":{
            "actual":[gross_actual/gross_barscale*bar_width, gross_actual], 
            "model":[gross_difference/gross_barscale*bar_width, gross_model],
            "exceeds_model":bool(gross_actual > gross_model),
            "difference": abs(gross_actual - gross_model)
        },
        # "solar":{"actual":486., "model":445},
    }

    #reformat all of the numbers
    for groupkey, item in context["production"].items():
        for key, subitem in item.items():
            if type(subitem) == type([]):
                out = []
                for v in subitem:
                    out.append(float("{:.1f}".format(v)))
            elif type(subitem) == type(1.0):
                out = float("{:.1f}".format(subitem))
            else:
                out = subitem
            context["production"][groupkey][key] = out
    print( context["production"] )

    #availability
    

    return render(request, "dashboard.html", context)

#-------------------------------------------------------------
def outlook(request, context={}):
    return render(request, "outlook.html", context)

#-------------------------------------------------------------
def planning(request, context={}):
    return render(request, "planning.html", context)

#-------------------------------------------------------------
def system(request, context={}):
    return render(request, "system.html", context)

#-------------------------------------------------------------
#-------------------------------------------------------------

def configure_sources(request, context={}):
    return render(request, "configure/sources.html", context)    

def configure_logging(request, context={}):
    return render(request, "configure/logging.html", context)    

def configure_model_settings(request, context={}):
    return render(request, "configure/model_settings.html", context)    

def configure_ui_settings(request, context={}):
    return render(request, "configure/ui_settings.html", context)    

def configure_event_log(request, context={}):
    return render(request, "configure/event_log.html", context)    

#-------------------------------------------------------------
#-------------------------------------------------------------

def whatif_interruptions(request, context={}):
    return render(request, "whatif/interruptions.html", context)

def whatif_resources(request, context={}):
    return render(request, "whatif/resources.html", context)

def whatif_weather(request, context={}):
    return render(request, "whatif/weather.html", context)