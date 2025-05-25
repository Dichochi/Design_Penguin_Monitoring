from django.contrib import admin
from .models import TaggedPenguin, Measurement

# Register your models here.
admin.site.register(TaggedPenguin)
admin.site.register(Measurement)
