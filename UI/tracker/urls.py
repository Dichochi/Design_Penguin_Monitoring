from django.urls import path
from . import views

urlpatterns = [
    path("index/", views.index, name="index"),
    path("", views.stats, name="stats"),
    path("penguins/", views.penguins, name="penguins"),
    path("<str:id>/<str:ts>", views.penguin_profile, name="penguin_profile"),
    path("add/", views.add, name="add"),
    path("about/", views.about, name="about"),
    path('api/weight-distribution/<str:time_range>/', views.weight_distribution_data, name='weight_distribution_data'),
    path("<str:id>/no-measurements/", views.no_measurements, name="no_measurements"),
]
