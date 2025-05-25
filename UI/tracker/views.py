from django.shortcuts import render
from django.http import HttpResponseRedirect
from django.urls import reverse
from .models import TaggedPenguin, Measurement
from django.db.models import OuterRef, Subquery, Avg, Max, Min, Count, Case, When, Value, CharField
from django.http import HttpResponse
import json
from django.http import JsonResponse
from django.views.decorators.csrf import csrf_exempt
from datetime import datetime, timezone, timedelta
from PIL import Image
import numpy as np
from django.db.models import Count
from django.db.models.functions import ExtractWeek, TruncWeek
from django.core.serializers.json import DjangoJSONEncoder
import json
from django.http import JsonResponse, HttpResponseBadRequest, HttpResponseForbidden

from zoneinfo import ZoneInfo
from django.utils import timezone  # Add this import at the top


API_KEY = "QWERTY"


# Create your views here.
def index(request):
    # Get measurements
    measurements = Measurement.objects.all()

    #TODO

    # Handle date filter
    date_filter = request.GET.get('date_filter', '')
    if date_filter:
        try:
            filter_date = datetime.strptime(date_filter, '%Y-%m-%d')
            measurements = measurements.filter(
                timestamp__year=filter_date.year,
                timestamp__month=filter_date.month,
                timestamp__day=filter_date.day
            )
        except ValueError:
            return JsonResponse({'error': 'Invalid Date Format'}, status=400)

    # Handle search
    search_query = request.GET.get('search', '')
    if search_query:
        measurements = measurements.filter(penguin__id__icontains=search_query)


    # Annotate each measurement with a status
    for measurement in measurements:
        if 2.2 <= measurement.weight <= 3.5:
            measurement.status = "ok"
        elif 0.5 <= measurement.weight <= 8:
            measurement.status = "warning"
        else:
            measurement.status = "critical"

    # Handle status filter
    status_filter = request.GET.get('status_filter', '')
    if status_filter:
        measurements = [m for m in measurements if m.status == status_filter]

    return render(request, "tracker/index.html", {
        "title": "Detection LOG",
        "measurements": measurements,
    })          

def penguins(request):
    # Get all penguins
    penguins = TaggedPenguin.objects.all()

    # Handle time filter
    time_filter = request.GET.get('time_filter', '')
    if time_filter:
        now = timezone.now()
        time_threshold = None

        if time_filter == '24h':
            time_threshold = now - timedelta(hours=24)
        elif time_filter == 'week':
            time_threshold = now - timedelta(weeks=1)
        elif time_filter == 'month':
            time_threshold = now - timedelta(days=30)
        elif time_filter == 'year':
            time_threshold = now - timedelta(days=365)

        if time_threshold:
            penguins = penguins.filter(
                measurements__timestamp__gte=time_threshold
            ).distinct()

    # Handle date filter
    date_filter = request.GET.get('date_filter', '')
    if date_filter:
        try:
            filter_date = datetime.strptime(date_filter, '%Y-%m-%d')
            penguins = penguins.filter(
                measurements__timestamp__year=filter_date.year,
                measurements__timestamp__month=filter_date.month,
                measurements__timestamp__day=filter_date.day
            ).distinct()
        except ValueError:
            return JsonResponse({'error': 'Invalid Timestamp'}, status=400)

    # Handle search
    search_query = request.GET.get('search', '')
    if search_query:
        penguins = penguins.filter(id__icontains=search_query)

    # Annotate with latest measurements and status
    now = timezone.now()
    penguins = penguins.annotate(
        latest_weight=Subquery(
            Measurement.objects.filter(
                penguin=OuterRef('pk')
            ).order_by('-timestamp').values('weight')[:1]
        ),
        latest_timestamp=Subquery(
            Measurement.objects.filter(
                penguin=OuterRef('pk')
            ).order_by('-timestamp').values('timestamp')[:1]
        ),
        measurement_count=Count('measurements'),
        status=Case(
            When(
                latest_timestamp__gte=now - timedelta(days=7),
                then=Value('ok')
            ),
            When(
                latest_timestamp__gte=now - timedelta(days=30),
                then=Value('unseen')
            ),
            default=Value('missing'),
            output_field=CharField()
        )
    )

    # Handle status filter
    status_filter = request.GET.get('status_filter', '')
    if status_filter:
        penguins = penguins.filter(status=status_filter)

    return render(request, "tracker/penguins.html", {
        "penguins": penguins
    })

def stats(request):
    penguins = TaggedPenguin.objects.all()
    measurements = Measurement.objects.all()
    first_five = Measurement.objects.all()[:5]
    total_penguins = TaggedPenguin.objects.count()
    total_measurements = Measurement.objects.count()

    # Calculate average weight across all measurements
    avg_weight = measurements.aggregate(Avg('weight'))['weight__avg']

    # Get count of measurements in last 24 hours
    twenty_four_hours_ago = timezone.now() - timedelta(hours=24)
    recent_measurements_count = measurements.filter(timestamp__gte=twenty_four_hours_ago).count()

    # Calculate weight distribution data
    weights = list(measurements.values_list('weight', flat=True))
    if weights:
        try:
            # Convert weights to float numpy array
            weights_array = np.array(weights, dtype=float)
            
            # Create histogram with 10 bins
            hist, bin_edges = np.histogram(weights_array, bins=10)
            
            # Serialize weight distribution data
            weight_distribution = {
                'counts': hist.tolist(),
                'bins': [f"{edge:.1f}" for edge in bin_edges[:-1]],
                'min_weight': float(min(weights_array)),
                'max_weight': float(max(weights_array))
            } 
        except (ValueError, TypeError) as e:
            # Handle invalid weight values
            weight_distribution = {
                'counts': [],
                'bins': [],
                'min_weight': 0,
                'max_weight': 0
            }
    else:
        weight_distribution = {
            'counts': [],
            'bins': [],
            'min_weight': 0,
            'max_weight': 0
        }

    # Annotate each measurement with a status
    for measurement in first_five:
        if 2.2 <= measurement.weight <= 3.5:
            measurement.status = "ok"
        elif 0.5 <= measurement.weight <= 8:
            measurement.status = "warning"
        else:
            measurement.status = "critical"

    # Serialize the data for safe use in JavaScript
    weight_distribution_json = json.dumps(weight_distribution, cls=DjangoJSONEncoder)
    
    return render(request, "tracker/stats.html", {
        "penguins": penguins,
        "measurements": measurements,
        "total_penguins": total_penguins,
        "total_measurements": total_measurements,
        "avg_weight": avg_weight,
        "recent_measurements_count": recent_measurements_count,
        "weight_distribution_json": weight_distribution_json,
        "first_five": first_five
    })

def about(request):
    return render(request, "tracker/about.html",)

def penguin_profile(request, id, ts):
    penguin = TaggedPenguin.objects.get(id=id)
    measurements = Measurement.objects.filter(penguin=penguin).order_by('-timestamp')

    if not measurements:
        return render(request, "tracker/No_measurements.html", {
            "penguin_id": penguin.pk
        })

    # Calculate current status based on latest measurement timestamp
    now = timezone.now()
    latest_measurement = measurements.first()

    if latest_measurement:
        days_since_last_seen = (now - latest_measurement.timestamp).days + 1
        print(days_since_last_seen)
        if days_since_last_seen <= 7:
            currentStatus = "ok"
        elif days_since_last_seen <= 30:
            currentStatus = "unseen"
        else:
            currentStatus = "missing"
    else:
        currentStatus = "missing"

    # Annotate each measurement with a status
    for measurement in measurements:
        if 2.2 <= measurement.weight <= 3.5:
            measurement.status = "ok"
        elif 0.5 <= measurement.weight <= 8:
            measurement.status = "warning"
        else:
            measurement.status = "critical"

    # Get weight statistics
    weight_stats = measurements.aggregate(
        avg_weight=Avg('weight'),
        max_weight=Max('weight'),
        min_weight=Min('weight'),
        count=Count('timestamp')
    )

    # Get weight history data for chart
    weight_history = measurements.values('timestamp', 'weight')

    # Calculate 30-day weight change
    latest = measurements.first()
    thirty_days_ago = measurements.filter(
        timestamp__lte=latest.timestamp - timedelta(days=30)
    ).first()

    weight_change = None
    if thirty_days_ago:
        weight_change = latest.weight - thirty_days_ago.weight

    # Calculate weekly detection counts for the last month
    last_month = timezone.now() - timedelta(days=30)
    weekly_counts = list(
        measurements.filter(timestamp__gte=last_month)
        .annotate(
            week=ExtractWeek('timestamp'),
            week_start=TruncWeek('timestamp')
        )
        .values('week', 'week_start')
        .annotate(count=Count('penguin_id'))
        .order_by('week_start')
    )

    context = {
        'penguin': penguin,
        'measurement': measurements.first(),
        'measurements': measurements,
        'weight_stats': weight_stats,
        'weight_change': weight_change,
        'weight_history': list(weight_history),
        'weekly_counts': weekly_counts,
        'weekly_counts_json': json.dumps(weekly_counts, cls=DjangoJSONEncoder),
        'currentStatus': currentStatus,  # Add currentStatus to context
    }

    return render(request, "tracker/penguin_profile.html", context)


@csrf_exempt
def add(request):
    print("Headers:", request.headers)
    print("POST data:", request.POST)
    print("FILES:", request.FILES)

    client_key = request.headers.get("X-API-KEY")
    if client_key != API_KEY:
        return HttpResponseForbidden("Invalid API key")

    if request.method == 'POST':
        try:
            # Parse JSON from the "data" field
            data = json.loads(request.POST.get("data", "{}"))
                

            timestamp = data.get("timestamp")
            weight = data.get("weight")
            penguin_id = data.get("id")


            # Get the uploaded image
            picture = request.FILES.get("picture")

            # if the image is invalid.
            if not picture:
                return JsonResponse({'error': 'No file uploaded'}, status=400)
            
            # Validate actual image content
            try:
                img = Image.open(picture)
                img.verify()  # This checks that it's not corrupted or spoofed
            except Exception:
                return JsonResponse({'error': 'Invalid or corrupted image'}, status=400)
            
            # Check the image format
            if img.format not in ['JPEG', 'PNG']:
                return JsonResponse({'error': 'Only JPEG and PNG formats are supported'}, status=400)
            
            # Reset file pointer (verify() moves it)
            picture.seek(0)

            
            # Check if penguin ID is Invalid.
            if (len(penguin_id) == 0):
                return JsonResponse({f'error': 'invalid penguinID = {penguin_id}'}, status=400)

            # Add to Data-Base
            if TaggedPenguin.objects.filter(id=penguin_id).exists(): # If penguin exits
                penguin = TaggedPenguin.objects.get(id=penguin_id)
            else:
                penguin = TaggedPenguin.objects.create(
                    id = penguin_id
                )

            # Create measurement
            measurement = Measurement.objects.create(
                timestamp = datetime.fromtimestamp(timestamp + 7200),
                weight=weight,
                penguin=penguin,
                image=picture
            )

            
            return JsonResponse({"status": "success"}, status=201)
        except Exception as e:
            return JsonResponse({"error": str(e)}, status=400)  
    
    else:
        return JsonResponse({"error": "Only POST allowed"}, status=405)

def weight_distribution_data(request, time_range):
    now = timezone.now()
    
    # Calculate start date based on time range
    if time_range == 'day':
        start_date = now - timedelta(days=1)
    elif time_range == 'week':
        start_date = now - timedelta(weeks=1)
    elif time_range == 'month':
        start_date = now - timedelta(days=30)
    elif time_range == 'year':
        start_date = now - timedelta(days=365)
    else:
        return JsonResponse({'error': 'Invalid time range'}, status=400)

    # Get filtered measurements
    measurements = Measurement.objects.filter(timestamp__gte=start_date)
    weights = list(measurements.values_list('weight', flat=True))

    if weights:
        # Create histogram data
        weights_array = np.array(weights, dtype=float)
        hist, bin_edges = np.histogram(weights_array, bins=10)
        data = {
            'counts': hist.tolist(),
            'bins': [f"{edge:.1f}" for edge in bin_edges[:-1]],
            'min_weight': float(min(weights_array)),
            'max_weight': float(max(weights_array))
        }
    else:
        data = {
            'counts': [],
            'bins': [],
            'min_weight': 0,
            'max_weight': 0
        }

    return JsonResponse(data)

def no_measurements(request, id):
    return render(request, "No_measurements.html", {
        "penguin_id": id
    })

